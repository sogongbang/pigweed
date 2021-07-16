# Copyright 2021 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

workspace(
    name = "pigweed",
    managed_directories = {"@npm": ["node_modules"]},
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("//pw_env_setup/bazel/cipd_setup:cipd_rules.bzl", "pigweed_deps")

# Setup CIPD client and packages.
# Required by: pigweed.
# Used by modules: all.
pigweed_deps()

load("@cipd_deps//:cipd_init.bzl", "cipd_init")

cipd_init()

# Setup python support.
# Required by: rules_fuzzing.
# Used in modules: None.
http_archive(
    name = "rules_python",
    sha256 = "778197e26c5fbeb07ac2a2c5ae405b30f6cb7ad1f5510ea6fdac03bded96cc6f",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_python/releases/download/0.2.0/rules_python-0.2.0.tar.gz",
        "https://github.com/bazelbuild/rules_python/releases/download/0.2.0/rules_python-0.2.0.tar.gz",
    ],
)

# Set up Starlark library
# Required by: io_bazel_rules_go, com_google_protobuf.
# Used in modules: None.
# This must be instantiated before com_google_protobuf as protobuf_deps() pulls
# in an older version of bazel_skylib. However io_bazel_rules_go requires a
# newer version.
http_archive(
    name = "bazel_skylib",
    sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

# Sets up host hermetic host toolchain.
# Required by: All cc targets.
# Used in modules: All cc targets.
git_repository(
    name = "rules_cc_toolchain",
    commit = "dd9265e3ce0daa444911040430bd716076869b34",
    remote = "https://github.com/silvergasp/rules_cc_toolchain.git",
)

load("@rules_cc_toolchain//:rules_cc_toolchain_deps.bzl", "rules_cc_toolchain_deps")

rules_cc_toolchain_deps()

load("@rules_cc_toolchain//cc_toolchain:cc_toolchain.bzl", "register_cc_toolchains")

register_cc_toolchains()

# Set up protobuf rules
# Required by: pigweed, com_github_bazelbuild_buildtools.
# Used in modules: //pw_protobuf.
http_archive(
    name = "com_google_protobuf",
    sha256 = "71030a04aedf9f612d2991c1c552317038c3c5a2b578ac4745267a45e7037c29",
    strip_prefix = "protobuf-3.12.3",
    url = "https://github.com/protocolbuffers/protobuf/archive/v3.12.3.tar.gz",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

# Setup tools to build custom grpc rules.
# Required by: pigweed.
# Used in modules: //pw_protobuf
http_archive(
    name = "rules_proto_grpc",
    sha256 = "5f0f2fc0199810c65a2de148a52ba0aff14d631d4e8202f41aff6a9d590a471b",
    strip_prefix = "rules_proto_grpc-1.0.2",
    urls = ["https://github.com/rules-proto-grpc/rules_proto_grpc/archive/1.0.2.tar.gz"],
)

load(
    "@rules_proto_grpc//:repositories.bzl",
    "rules_proto_grpc_repos",
    "rules_proto_grpc_toolchains",
)

rules_proto_grpc_toolchains()

rules_proto_grpc_repos()

# Set up build_bazel_rules_nodejs.
# Required by: pigweed.
# Used in modules: //pw_web_ui.
git_repository(
    name = "platforms",
    commit = "d4c9d7f51a7c403814b60f66d20eeb425fbaaacb",
    remote = "https://github.com/bazelbuild/platforms.git",
)

http_archive(
    name = "build_bazel_rules_nodejs",
    sha256 = "8f5f192ba02319254aaf2cdcca00ec12eaafeb979a80a1e946773c520ae0a2c9",
    urls = ["https://github.com/bazelbuild/rules_nodejs/releases/download/3.7.0/rules_nodejs-3.7.0.tar.gz"],
)

# Get the latest LTS version of Node.
load(
    "@build_bazel_rules_nodejs//:index.bzl",
    "node_repositories",
    "yarn_install",
)

node_repositories(package_json = ["//:package.json"])

yarn_install(
    name = "npm",
    package_json = "//:package.json",
    yarn_lock = "//:yarn.lock",
)

http_archive(
    name = "io_bazel_rules_webtesting",
    sha256 = "9bb461d5ef08e850025480bab185fd269242d4e533bca75bfb748001ceb343c3",
    urls = ["https://github.com/bazelbuild/rules_webtesting/releases/download/0.3.3/rules_webtesting.tar.gz"],
)

load("@io_bazel_rules_webtesting//web:repositories.bzl", "web_test_repositories")

web_test_repositories()

load(
    "@io_bazel_rules_webtesting//web/versioned:browsers-0.3.2.bzl",
    "browser_repositories",
)

browser_repositories(
    chromium = True,
    firefox = True,
)

# Setup embedded C/C++ toolchains.
# Required by: pigweed.
# Used in modules: //pw_polyfill, //pw_build (all pw_cc* targets).
git_repository(
    name = "bazel_embedded",
    commit = "7dd51fc1ac7d7a9ecfe8ea8d1532c56a6ca836bc",
    remote = "https://github.com/silvergasp/bazel-embedded.git",
)

# Instantiate Pigweed configuration for embedded toolchain,
# this must be called before bazel_embedded_deps.
load(
    "//pw_build:pigweed_toolchain_upstream.bzl",
    "toolchain_upstream_deps",
)

toolchain_upstream_deps()

# Configure bazel_embedded toolchains and platforms.
load(
    "@bazel_embedded//:bazel_embedded_deps.bzl",
    "bazel_embedded_deps",
)

bazel_embedded_deps()

load(
    "@bazel_embedded//platforms:execution_platforms.bzl",
    "register_platforms",
)

register_platforms()

# Fetch gcc-arm-none-eabi compiler and register for toolchain
# resolution.
load(
    "@bazel_embedded//toolchains/compilers/gcc_arm_none_eabi:gcc_arm_none_repository.bzl",
    "gcc_arm_none_compiler",
)

gcc_arm_none_compiler()

load(
    "@bazel_embedded//toolchains/gcc_arm_none_eabi:gcc_arm_none_toolchain.bzl",
    "register_gcc_arm_none_toolchain",
)

register_gcc_arm_none_toolchain()

# Registers platforms for use with toolchain resolution
register_execution_platforms("//pw_build/platforms:all")

# Setup Golang toolchain rules
# Required by: bazel_gazelle, com_github_bazelbuild_buildtools.
# Used in modules: None.
http_archive(
    name = "io_bazel_rules_go",
    sha256 = "d1ffd055969c8f8d431e2d439813e42326961d0942bdf734d2c95dc30c369566",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.24.5/rules_go-v0.24.5.tar.gz",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.24.5/rules_go-v0.24.5.tar.gz",
    ],
)

load(
    "@io_bazel_rules_go//go:deps.bzl",
    "go_register_toolchains",
    "go_rules_dependencies",
)

go_rules_dependencies()

go_register_toolchains()

# Setup bazel package manager for golang
# Required by: com_github_bazelbuild_buildtools.
# Used in modules: None
http_archive(
    name = "bazel_gazelle",
    sha256 = "b85f48fa105c4403326e9525ad2b2cc437babaa6e15a3fc0b1dbab0ab064bc7c",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.22.2/bazel-gazelle-v0.22.2.tar.gz",
        "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.22.2/bazel-gazelle-v0.22.2.tar.gz",
    ],
)

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

gazelle_dependencies()

# Setup bazel buildtools (bazel linter and formatter)
# Required by: pigweed
# Used in modules: //:all (bazel specific tools).
http_archive(
    name = "com_github_bazelbuild_buildtools",
    sha256 = "c28eef4d30ba1a195c6837acf6c75a4034981f5b4002dda3c5aa6e48ce023cf1",
    strip_prefix = "buildtools-4.0.1",
    url = "https://github.com/bazelbuild/buildtools/archive/4.0.1.tar.gz",
)

load("//pw_build:target_config.bzl", "pigweed_config")

# Configure Pigweeds backend.
pigweed_config(
    name = "pigweed_config",
    build_file = "//targets:default_config.BUILD",
)

# Setup rules_fuzzing for fuzz tests.
# Required by: pigweed.
# Used in modules: pw_protobuf, pw_tokenizer, pw_fuzzer.
http_archive(
    name = "rules_fuzzing",
    sha256 = "94f25c7a18db0502ace26a3ef7d0a25fd7c195c4e9770ddd1b1ec718e8936091",
    strip_prefix = "rules_fuzzing-0.1.3",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.1.3.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()
