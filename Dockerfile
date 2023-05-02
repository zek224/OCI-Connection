# Use a base image with a C compiler and necessary tools
FROM gcc:latest

# Set the working directory
WORKDIR /app

# Copy the wallet, instantclient, and server code into the container
COPY wallet/ ./wallet/
COPY instantclient/ ./instantclient/
COPY Server/ ./Server/
COPY key.txt ./key.txt

# Install any necessary dependencies (OCI libraries in this case)
RUN apt-get update && \
    apt-get install -y libaio1 libaio-dev && \
    # Set the environment variables for the Oracle Instant Client
    echo 'export LD_LIBRARY_PATH=/app/instantclient' >> ~/.bashrc && \
    echo 'export TNS_ADMIN=/app/wallet' >> ~/.bashrc && \
    . ~/.bashrc

# Compile the server code using the Makefile
WORKDIR /app/Server
RUN make

# Set the entrypoint for the container
ENTRYPOINT ["./server"]

# Handle SIGINT signal gracefully
STOPSIGNAL SIGINT
