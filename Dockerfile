# Use Alpine as a base image
FROM alpine:latest

# Install build dependencies (make, g++, etc.)
RUN apk update && \
    apk add --no-cache \
    build-base \
    make \
    g++

# Set the working directory inside the container
WORKDIR /workspace

# Expose the working directory as a volume
VOLUME ["/workspace"]

# Default command to run bash
CMD ["/bin/sh"]
