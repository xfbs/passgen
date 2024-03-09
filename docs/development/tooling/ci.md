# CI/CD

A CI/CD system (*continuous integration* and *continuous deployment*) is used
to run jobs every time the code changes, such as when new code is pushed
or in merge requests. These jobs are used to check the changes (*integration*)
and deploy them (*deployment*). 

In the case of Passgen, the system is used to run a whole array of checks
against the code (unit tests, including running them with Valgrind or LLVM
sanitizers), generate reports (test coverage, source code documentation) and
deploy those as well as this documentation.

This project uses GitLab Pipelines as its CI. These are configured by the
[`.gitlab-ci.yml`][gitlabciyml] file. This file defines that jobs that are run,
and when they are supposed to be run. GitLab CI makes it very easy to define
jobs because it has a very simple YAML syntax and the jobs run in Docker
containers, which makes it easy to test and debug them locally.

The CI configuration is split into different stages. Passgen uses four stages
in its config: *Setup*, *Test*, *Build*, *Deploy*.

These stages will run sequentially, while all jobs that are part of a given
stage run in parallel. This means that first, all of the jobs from the *setup*
stage will run in parallel. Once they have all completed, then all of the jobs
from the *test* stage will run in parallel. Once they have completed, all of
the jobs from the *build* stage will run, and so on.

Every job runs in a Docker container, has access to the repository and
artifacts of the previous jobs. Every job can export some artifacts. Jobs can
also create Docker images, more about that later. For example, this is the job
that builds Passgen for amd64:

```yaml
build:amd64:
  image: registry.gitlab.com/xfbs/passgen/amd64
  stage: build
  script:
    - mkdir build-amd64 && cd build-amd64
    - cmake -GNinja -DCMAKE_UNITY_BUILD=YES -DCMAKE_BUILD_TYPE=Release -DPASSGEN_SECCOMP=YES ..
    - ninja
    - ./src/tests/passgen-test
    - ../scripts/ci-test-passgen.sh ./src/cli/passgen
    - cpack -G TXZ
    - cpack -G DEB
  artifacts:
    paths:
      - build-amd64
    expire_in: 1 week
```

This job defines that it should use the Docker image
`registry.gitlab.com/xfbs/passgen/adm64`, it is part of the *build* stage, it
defines some commands that should be run in this Docker image and finally it
exports the `build-amd64` folder as an artifact.

You can think of jobs as a function mentally: they can take some inputs (the
code, artifacts from previous jobs) and produce some outputs (new artifacts).
This is why GitLab also calls them *pipelines*.

The following sections explain how Passgen uses GitLab CI, and what the
individual jobs do.

## Setup

In the setup stage, some jobs are run that create Docker containers which are
later used by other jobs. These Docker containers are defined by Dockerfiles
located in the `scripts` folder. It is very easy to create custom Docker images
as part of GitLab CI jobs because GitLab has a Container Registry built-in that
CI jobs have access to.

Here is how one of these jobs looks like:

```yaml
docker:alpine:
  image: docker
  stage: setup
  services:
    - docker:dind
  script:
    - docker login -u "$CI_REGISTRY_USER" -p "$CI_REGISTRY_PASSWORD" $CI_REGISTRY
    - docker build . -f scripts/alpine.dockerfile -t "$CI_REGISTRY_IMAGE/alpine"
    - docker push "$CI_REGISTRY_IMAGE/alpine"
  only:
    refs:
      - master
    changes:
      - scripts/alpine.dockerfile
```

You can see that this job uses the `docker` image. It requires the
`docker:dind` service, this is simply another Docker container that is spawned
and runs a Docker daemon. Using these two together allows the job to use Docker
in the CI job. It logs in to the GitLab registry (`registry.gitlab.com`) and
builds and pushes the `alpine.dockerfile` image. This will end up at
`registry.gitlab.com/xfbs/passgen/alpine`.

What you can also see is the `only` section. This limits the job to only
running for new code on the `master` branch, and to only running if the
`alpine.dockerfile` has changed in the commit.

These are the jobs in the *setup* stage and what they do:

| Name | Description |
| --- | --- |
| `docker:alpine` | Builds `alpine.dockerfile`. Used to build static MUSL releases of Passgen. |
| `docker:deploy` | Builds `deploy.dockerfile`. Used to build documentation and deploy to GitLab Pages. |
| `docker:mingw64` | Builds `mingw64.dockerfile`. Used to cross-compile Passgen for Windows. |
| `docker:macos` | Builds `macos.dockerfile`. Used to cross-compile Passgen for macOS. |
| `docker:riscv64` | Builds `riscv64.dockerfile`. Used for cross-compile RISC-V releases of Passgen. |
| `docker:amd64` | Builds `amdm64.dockerfile`. Used to build AMD64 releases of Passgen. |
| `docker:aarch64` | Builds `aarch64.dockerfile`. Used to cross-compile ARM releases of Passgen. |
| `docker:wasi` | Builds `wasi.dockerfile`. Used to create WASI WebAssembly releases of Passgen. |

While the jobs are set up to only run if the respective Dockerfile has changed,
there is also a [scheduled pipeline][schedule] to rebuild everything weekly.
This causes these jobs to run regardless, in order to ensure that these images
do not get stale.

[schedule]: https://gitlab.com/xfbs/passgen/-/pipeline_schedules

## Test

In the *test* stage, Passgen is built in debug mode and various unit tests are run against it. For jobs that cross-compile, QEMU is used to emulate other architectures (such as ARM or RISC-V64) to be able to run the unit tests.

Here is a list of the jobs in this stage and what they do:

| Name | Description |
| --- | --- |
| `sanitize` | Builds Passgen using the `amd64` image with various LLVM sanitizers enabled and runs unit tests. |
| `test:aarch64` | Builds Passgen for ARM using the `aarch64` and runs unit tests with QEMU. |
| `test:alpine` | Builds Passgen statically using the `alpine` image and runs unit tests. |
| `test:amd64` | Builds Passgen using the `amd64` image and runs unit tests. |
| `test:riscv64` | Builds Passgen using the `riscv64` image and runs unit tests with QEMU. |

## Build

In the *build* stage, Passgen is built in release mode, and it is packaged.

## Deploy

In the *deploy* stage, documentation is built and published on GitLab Pages, along with the releases from the *build* stage.

[gitlabciyml]: https://gitlab.com/xfbs/passgen/-/blob/master/.gitlab-ci.yml
