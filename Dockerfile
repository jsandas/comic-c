# Docker image for building Captain Comic C refactor
# Provides Open Watcom 2, NASM, and djlink for DOS real-mode compilation

FROM debian:bullseye-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    curl \
    unzip \
    make \
    gcc \
    g++ \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Install NASM assembler
RUN apt-get update && apt-get install -y nasm && rm -rf /var/lib/apt/lists/*

# Install Open Watcom 2
# Download and extract Open Watcom v2 (Linux x64 version)
WORKDIR /opt/watcom
RUN curl -s -OL https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/ow-snapshot.tar.xz \
    && tar xJf ow-snapshot.tar.xz \
    && rm ow-snapshot.tar.xz

# Set up Open Watcom environment variables
ENV WATCOM=/opt/watcom
ENV PATH=$WATCOM/binl64:$WATCOM/binl:$PATH
ENV INCLUDE=$WATCOM/h
ENV EDPATH=$WATCOM/eddat
ENV WIPFC=$WATCOM/wipfc

# Set working directory to workspace mount point
WORKDIR /workspace
