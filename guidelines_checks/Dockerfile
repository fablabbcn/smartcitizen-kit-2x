FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && \
    apt-get install -y clang clang-tidy clang-format build-essential cmake git python3 python3-pip

# Set workdir
WORKDIR /workspace

COPY run-clang-checks.sh /workspace/run-clang-checks.sh

# Default command: run clang checks
CMD ["sh", "/workspace/run-clang-checks.sh"]