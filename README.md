# OCI-Connection
For CBDC Capstone project

Build Docker container:
``` 
docker build -t db-server-image . 
```

Run Docker container:
``` 
docker run -p 8080:8080 -d --name db-server db-server-image 
```

Stop Docker container:
``` 
docker stop db-server 
```
