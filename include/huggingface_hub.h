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

/**
 * @file huggingface_hub.h
 * @brief Header file for Hugging Face Hub C++ API.
 *
 * This file contains the declarations of the structures and functions used to
 * interact with the Hugging Face Hub.
 */

#ifndef HUGGINGFACE_HUB_H
#define HUGGINGFACE_HUB_H

#include <stdint.h>
#include <string>
#include <variant>

namespace huggingface_hub {
/**
 * @struct FileMetadata
 * @brief Structure to hold metadata of a file.
 *
 * This structure contains the SHA-256 hash, commit ID, and size of a file.
 */
struct FileMetadata {
  std::string commit; /**< Commit ID of the file */
  std::string type;   /**< Type of the file (e.g., "model", "dataset") */
  std::string oid;    /**< Object ID of the file */
  uint64_t size;      /**< Size of the file in bytes */
  std::string sha256; /**< SHA-256 hash of the file */
};

/**
 * @struct DownloadResult
 * @brief Structure to hold the result of a download operation.
 *
 * This structure contains the success status and the path of the downloaded
 * file.
 */
struct DownloadResult {
  bool success;     /**< Indicates if the download was successful */
  std::string path; /**< Path to the downloaded file */
};

/**
 * @brief Get metadata of a model file from Hugging Face Hub.
 *
 * This function retrieves the metadata of a specified file from a given
 * repository on the Hugging Face Hub.
 *
 * @param repo The repository name or ID.
 * @param file The file name within the repository.
 * @return A variant containing either the FileMetadata structure or an error
 * message string.
 */
std::variant<struct FileMetadata, std::string>
get_model_metadata_from_hf(const std::string &repo, const std::string &file);

/**
 * @brief Download a file from Hugging Face Hub.
 *
 * This function downloads a specified file from a given repository on the
 * Hugging Face Hub and saves it to the specified cache directory.
 *
 * @param repo_id The repository ID.
 * @param filename The name of the file to download.
 * @param cache_dir The directory to cache the downloaded file. Default is
 * "~/.cache/huggingface/hub".
 * @param force_download If true, forces the download even if the file already
 * exists in the cache.
 * @return A DownloadResult structure containing the success status and the path
 * of the downloaded file.
 */
struct DownloadResult
hf_hub_download(const std::string &repo_id, const std::string &filename,
                const std::string &cache_dir = "~/.cache/huggingface/hub",
                bool force_download = false, bool verbose = false);

/**
 * @brief Download a file from Hugging Face Hub.
 *
 * This function downloads a specified file from a given repository on the
 * Hugging Face Hub and saves it to the specified cache directory.
 *
 * @param repo_id The repository ID.
 * @param filename The name of the file to download.
 * @param cache_dir The directory to cache the downloaded file. Default is
 * "~/.cache/huggingface/hub".
 * @param force_download If true, forces the download even if the file already
 * exists in the cache.
 * @return A DownloadResult structure containing the success status and the path
 * of the downloaded file.
 */
struct DownloadResult hf_hub_download_with_shards(
    const std::string &repo_id, const std::string &filename,
    const std::string &cache_dir = "~/.cache/huggingface/hub",
    bool force_download = false);

#endif // HUGGINGFACE_HUB_H
} // namespace huggingface_hub