FROM ubuntu:latest

# Install necessary dependencies
RUN apt update && apt install -y g++ gcc make

# Set working directory
WORKDIR /workspace

# Expose the working directory as a volume
VOLUME ["/workspace"]

CMD ["/bin/bash"]