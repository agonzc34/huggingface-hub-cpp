#include "huggingface_hub.h"
#include <iostream>

int main() {
  std::string repo_id = "rhasspy/piper-voices";
  std::string filename = "en/en_US/lessac/low/en_US-lessac-low.onnx.json";

  if (huggingface_hub::hf_hub_download(repo_id, filename).success) {
    std::cout << "Downloaded " << filename << " from " << repo_id << std::endl;
  } else {
    std::cout << "Failed to download " << filename << " from " << repo_id
              << std::endl;
  }
}