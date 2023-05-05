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

# SQL cURL statements
SELECT
```
curl -X POST -d "select * from test order by id" --max-time 10 http://localhost:8080
```
INSERT
```
curl -X POST -d "insert into test values (10,'test10')" --max-time 10 http://localhost:8080
```
DELETE
```
curl -X POST -d "delete from zek224.test where id=10" --max-time 10 http://localhost:8080
```
