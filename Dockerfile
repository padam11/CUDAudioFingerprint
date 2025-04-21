FROM nvidia/cuda:12.2.0-devel-ubuntu22.04

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    nano \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy your entire project
COPY . .

# Optional: build on image build
# RUN make

CMD ["make"]
