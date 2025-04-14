// MIT License
//
// Copyright (c) 2025 Alejandro González Cantón
// Copyright (c) 2025 Miguel Ángel González Santamarta
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

#include <curl/curl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "huggingface_hub.h"

namespace huggingface_hub {

volatile sig_atomic_t stop_download = 0;
void handle_sigint(int) { stop_download = 1; }

bool log_verbose = false;

void log_debug(const std::string &message) {
  if (!log_verbose) {
    return;
  }
  fprintf(stderr, "\036[31m[DEBUG] %s\033[0m\n", message.c_str());
  fflush(stderr);
}

void log_info(const std::string &message) {
  fprintf(stderr, "[INFO] %s\n", message.c_str());
  fflush(stderr);
}

void log_info_with_carriage_return(const std::string &message) {
  fprintf(stderr, "\r\033[1A\033[2K"); // Move up and clear line
  fprintf(stderr, "[INFO] %s\n", message.c_str());
  fflush(stderr);
}

void log_error(const std::string &message) {
  fprintf(stderr, "\033[31m[ERROR] %s\033[0m\n", message.c_str());
  fflush(stderr);
}

long get_file_size(const std::string &filename) {
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) == 0) {
    return stat_buf.st_size;
  }
  return 0; // File doesn't exist
}

std::filesystem::path expand_user_home(const std::string &path) {
  if (!path.empty() && path[0] == '~') {
    const char *home = std::getenv("HOME"); // Get HOME environment variable
    if (home) {
      return std::filesystem::path(home + path.substr(1));
    }
  }
  return std::filesystem::path(path);
}

std::string create_cache_system(const std::string &cache_dir,
                                const std::string &repo_id) {
  std::string model_folder = std::string("models/" + repo_id);

  size_t pos = 0;
  while ((pos = model_folder.find("/", pos)) != std::string::npos) {
    model_folder.replace(pos, 1, "--");
    pos += 2;
  }

  std::string expanded_cache_dir = expand_user_home(cache_dir);

  std::string model_cache_path = expanded_cache_dir + "/" + model_folder + "/";

  std::string refs_path = model_cache_path + std::string("refs");
  std::string blobs_path = model_cache_path + std::string("blobs");
  std::string snapshots_path = model_cache_path + std::string("snapshots");

  std::filesystem::create_directories(refs_path);
  std::filesystem::create_directories(blobs_path);
  std::filesystem::create_directories(snapshots_path);

  return model_cache_path;
}

size_t write_string_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  if (!stream) {
    log_error("Error: stream is null!");
    return 0;
  }
  std::string *out = static_cast<std::string *>(stream);
  out->append(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

size_t write_file_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  if (!stream) {
    log_error("Error: stream is null!");
    return 0;
  }
  std::ofstream *out = static_cast<std::ofstream *>(stream);
  if (!out->is_open()) {
    log_error("Error: output file stream is not open!");
    return 0;
  }
  out->write(static_cast<char *>(ptr), size * nmemb);
  return size * nmemb;
}

// Extract metadata from JSON response
FileMetadata extract_metadata(const std::string &json) {
  FileMetadata metadata;

  std::smatch match;

  // Extract "type"
  if (std::regex_search(json, match,
                        std::regex(R"(\"type\"\s*:\s*\"([^"]+)\")")))
    metadata.type = match[1];

  // Extract "oid" (top-level one)
  if (std::regex_search(json, match,
                        std::regex(R"(\"oid\"\s*:\s*\"([a-f0-9]{40})\")")))
    metadata.oid = match[1];

  // Extract "size"
  if (std::regex_search(json, match, std::regex(R"(\"size\"\s*:\s*(\d+))")))
    metadata.size = std::stoull(match[1]);

  // Extract "lfs" SHA-256 hash
  if (std::regex_search(
          json, match,
          std::regex(
              R"(\"lfs\"\s*:\s*\{[^}]*\"oid\"\s*:\s*\"([a-f0-9]{64})\")")))
    metadata.sha256 = match[1];

  // Extract "commit" ID
  if (std::regex_search(
          json, match,
          std::regex(
              R"(\"lastCommit\"\s*:\s*\{[^}]*\"id\"\s*:\s*\"([a-f0-9]{40})\")")))
    metadata.commit = match[1];

  return metadata;
}

std::variant<struct FileMetadata, std::string>
get_model_metadata_from_hf(const std::string &repo, const std::string &file) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "Failed to initialize CURL";
  }

  std::string response, headers;

  std::string url =
      "https://huggingface.co/api/models/" + repo + "/paths-info/main";
  const std::string body = "{\"paths\": [\"" + file + "\"], \"expand\": true}";

  struct curl_slist *http_headers = NULL;
  http_headers =
      curl_slist_append(http_headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(http_headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return "CURL request failed: " + std::string(curl_easy_strerror(res));
  }

  return extract_metadata(response);
}

int get_terminal_width() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
    return w.ws_col;
  } else {
    return 80; // Default terminal width
  }
}

std::chrono::steady_clock::time_point last_print_time =
    std::chrono::steady_clock::now();

// Progress bar function
int progress_callback(void *userdata, curl_off_t total, curl_off_t now,
                      curl_off_t, curl_off_t) {
  static auto start_time = std::chrono::steady_clock::now();
  struct FileMetadata *metadata = static_cast<struct FileMetadata *>(userdata);
  uint64_t size = metadata->size;
  uint64_t byte_offset = total - size;
  uint64_t downloaded = now - byte_offset;
  int terminal_width = get_terminal_width();
  auto elapsed = std::chrono::steady_clock::now() - last_print_time;

  if (total > 0 && (now == downloaded ||
                    std::chrono::duration<double>(elapsed).count() > 0.08)) {
    last_print_time = std::chrono::steady_clock::now();

    bool show_speed = terminal_width > 65;
    int width = terminal_width - 65 + (show_speed ? 0 : 10);
    float percent = static_cast<float>(downloaded) / size;
    int filled = static_cast<int>(percent * width);

    auto elapsed = std::chrono::steady_clock::now() - start_time;
    double speed = now / (std::chrono::duration<double>(elapsed).count() +
                          1e-6); // Avoid division by zero
    double remaining = (total - now) / speed;

    std::ostringstream progress;

    // Print progress bar
    if (terminal_width > 50) {
      progress << "[";
      for (int i = 0; i < filled; ++i)
        progress << "#";
      for (int i = filled; i < width; ++i)
        progress << " ";
      progress << "] ";
    }
    progress << std::fixed << std::setprecision(2) << (percent * 100) << "%";

    progress << "   " << downloaded / 1024 / 1024 << " MB / "
             << size / 1024 / 1024 << " MB ";

    if (show_speed) {
      progress << " " << (speed / 1024 / 1024) << " MB/s ";
    }
    progress << " | ETA: " << std::fixed << std::setprecision(1) << remaining
             << "s";
    log_info_with_carriage_return(progress.str());
  }
  if (stop_download) {
    return 1; // Non-zero return value cancels the transfer
  }

  return 0; // Continue downloading
}

CURLcode perform_download(std::string url,
                          std::string blob_incomplete_file_path,
                          bool force_download, struct FileMetadata metadata) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return CURLE_FAILED_INIT;
  }

  std::ofstream file(blob_incomplete_file_path,
                     std::ios::binary | std::ios::app);

  if (!file.is_open()) {
    log_error("Error: failed to open file stream!");
    return CURLE_FAILED_INIT;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());   // Set URL
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   write_file_data);                // Write data to file
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file); // File stream
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);   // Enable progress callback
  curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                   progress_callback); // Progress callback
  curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &metadata);

  // Resume download if file exists
  long existing_size = get_file_size(blob_incomplete_file_path);
  if (existing_size > 0 && !force_download) {
    curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE,
                     (curl_off_t)existing_size);
    log_info("Resuming download from " + std::to_string(existing_size) +
             " bytes...");
  }

  fprintf(stderr, "\n"); // New line after progress bar
  CURLcode res = curl_easy_perform(curl);
  fprintf(stderr, "\n"); // New line after progress bar
  curl_easy_cleanup(curl);
  file.close();
  return res;
}

struct DownloadResult hf_hub_download(const std::string &repo_id,
                                      const std::string &filename,
                                      const std::string &cache_dir,
                                      bool force_download, bool verbose) {
  signal(SIGINT, handle_sigint);
  log_verbose = verbose;

  struct DownloadResult result;
  result.success = true;

  // 1. Check that model exists on Hugging Face
  auto metadata_result = get_model_metadata_from_hf(repo_id, filename);
  if (std::holds_alternative<std::string>(metadata_result)) {
    log_error(std::get<std::string>(metadata_result));
    result.success = false;
    return result;
  }

  // 2. Create Cache Dir Struct
  std::string cache_model_dir = create_cache_system(cache_dir, repo_id);
  log_debug("Cache directory: " + cache_model_dir);
  log_info("Downloading " + filename + " from " + repo_id);

  struct FileMetadata metadata = std::get<struct FileMetadata>(metadata_result);
  log_debug("Commit: " + metadata.commit);
  log_debug("Blob ID: " + metadata.oid);
  log_debug("Size: " + std::to_string(metadata.size) + " bytes");
  log_debug("SHA256: " + metadata.sha256);

  std::filesystem::path blob_file_path;
  std::filesystem::path blob_incomplete_file_path;

  if (metadata.sha256.empty()) {
    blob_file_path = cache_model_dir + "blobs/" + metadata.oid;
    blob_incomplete_file_path =
        cache_model_dir + "blobs/" + metadata.oid + ".incomplete";
  } else {
    blob_file_path = cache_model_dir + "blobs/" + metadata.sha256;
    blob_incomplete_file_path =
        cache_model_dir + "blobs/" + metadata.sha256 + ".incomplete";
  }

  std::filesystem::path snapshot_file_path(cache_model_dir + "snapshots/" +
                                           metadata.commit + "/" + filename);
  std::filesystem::path refs_file_path(cache_model_dir + "refs/main");

  result.path = snapshot_file_path;

  if (std::filesystem::exists(snapshot_file_path) &&
      std::filesystem::exists(blob_file_path) && !force_download) {
    log_info("Snapshot file exists. Skipping download...");
    return result;
  }

  if (std::filesystem::exists(refs_file_path)) {
    std::ifstream refs_file(refs_file_path);
    std::string commit;
    refs_file >> commit;
    refs_file.close();
  } else {
    std::ofstream refs_file(refs_file_path);
    refs_file << metadata.commit;
    refs_file.close();
  }

  // 3. Download the file
  std::string url =
      "https://huggingface.co/" + repo_id + "/resolve/main/" + filename;
  std::filesystem::create_directories(snapshot_file_path.parent_path());

  if (!std::filesystem::exists(blob_file_path) || force_download) {
    CURLcode res = perform_download(url, blob_incomplete_file_path,
                                    force_download, metadata);
    result.success = res == CURLE_OK;

    if (stop_download) {
      log_info("Download interrupted. Exiting...");
      return result;
    } else if (!result.success) {
      log_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
      return result;
    } else {
      std::filesystem::rename(blob_incomplete_file_path, blob_file_path);
    }
  }

  if (std::filesystem::exists(snapshot_file_path)) {
    log_debug("Snapshot file exists. Deleting...");
    std::filesystem::remove(snapshot_file_path);
  }
  std::filesystem::create_symlink(blob_file_path, snapshot_file_path);

  log_info("Downloaded to: " + snapshot_file_path.string());

  result.success = true;
  return result;
}

struct DownloadResult hf_hub_download_with_shards(const std::string &repo_id,
                                                  const std::string &filename,
                                                  const std::string &cache_dir,
                                                  bool force_download) {

  std::regex pattern(R"(-([0-9]+)-of-([0-9]+)\.(\w+))");
  std::smatch match;

  if (std::regex_search(filename, match, pattern)) {
    int total_shards = std::stoi(match[2]);
    std::string base_name = filename.substr(0, match.position(0));
    std::string extension = match[3];

    // Download shards
    for (int i = 1; i <= total_shards; ++i) {
      char shard_file[512];
      snprintf(shard_file, sizeof(shard_file), "%s-%05d-of-%05d.%s",
               base_name.c_str(), i, total_shards, extension.c_str());
      auto aux_res =
          hf_hub_download(repo_id, shard_file, cache_dir, force_download);

      if (!aux_res.success) {
        return aux_res;
      }
    }

    // Return first shard
    char first_shard[512];
    snprintf(first_shard, sizeof(first_shard), "%s-00001-of-%05d.%s",
             base_name.c_str(), total_shards, extension.c_str());
    return hf_hub_download(repo_id, first_shard, cache_dir, false);
  }

  return hf_hub_download(repo_id, filename, cache_dir, force_download);
}

} // namespace huggingface_hub