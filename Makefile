DOCKER=docker
DOCKER_IMAGE=registry.gitlab.com/xfbs/passgen

docker:
	$(DOCKER) build scripts -f scripts/riscv64.dockerfile -t $(DOCKER_IMAGE)/riscv64
	$(DOCKER) build scripts -f scripts/aarch64.dockerfile -t $(DOCKER_IMAGE)/aarch64
	$(DOCKER) build scripts -f scripts/mingw64.dockerfile -t $(DOCKER_IMAGE)/mingw64
	$(DOCKER) build scripts -f scripts/amd64.dockerfile -t $(DOCKER_IMAGE)/amd64

docker-push:
	$(DOCKER) push $(DOCKER_IMAGE)/riscv64
	$(DOCKER) push $(DOCKER_IMAGE)/aarch64
