// Copyright 2023 TikTok Pte. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "example.h"

#include "gflags/gflags.h"

DEFINE_string(config_path, "./json/ecdh_psi_sender.json", "the path where the sender's config file located");
DEFINE_bool(use_random_data, true, "use randomly generated data or read data from files.");
DEFINE_string(log_path, "./logs/", "the directory where log file located");
// The following two variables only make sense if you use random data.
DEFINE_uint64(intersection_size, 10, "the intersection size of both party.");
DEFINE_uint64(intersection_ratio, 10, "the ratio of sender/receiver data size to intersection size.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    ecdh_psi_example(FLAGS_config_path, FLAGS_log_path, FLAGS_use_random_data, FLAGS_intersection_size,
            FLAGS_intersection_ratio);
    return 0;
}
