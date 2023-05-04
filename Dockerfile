# Use the Oracle Linux 8 base image
FROM oraclelinux:8-slim

# Set the working directory
WORKDIR /app

# Copy the wallet, instantclient, and server code into the container
COPY wallet/ ./wallet/
COPY instantclient-basic-linux.x64-21.10.0.0.0dbru.zip ./instantclient.zip
COPY instantclient-sdk-linux.x64-21.10.0.0.0dbru.zip ./instantclient-sdk.zip
COPY Server/* ./Server/
COPY key.txt ./key.txt

# Install necessary dependencies
RUN microdnf update -y && \
    microdnf install -y libaio gcc make unzip && \
    microdnf clean all

# RUN ls -l /app/

# Extract the Instant Client files
RUN unzip instantclient.zip && \
    unzip instantclient-sdk.zip && \
    mv instantclient_21_10 instantclient

# Set the OCI library environment variables
ENV LD_LIBRARY_PATH=/app/instantclient
ENV PATH=$PATH:/app/instantclient

# RUN ls -l /app/

# Compile the server code
WORKDIR /app/Server
RUN make

# Set the entrypoint for the container
ENTRYPOINT ["./server"]

# Handle SIGINT signal gracefully
STOPSIGNAL SIGINT

# Expose the port that the server is listening on
EXPOSE 8080
