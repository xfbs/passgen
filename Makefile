DOCKER=docker
DOCKER_IMAGE=registry.gitlab.com/xfbs/passgen

docker:
	$(DOCKER) build scripts -f scripts/riscv64.dockerfile -t $(DOCKER_IMAGE)/riscv64

docker-push:
	$(DOCKER) push $(DOCKER_IMAGE)/riscv64
