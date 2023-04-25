import os, oracledb

# get keys from key file
username, password, wallet_pw = "", "", ""

# read file for passcodes and username
with open('key.txt', 'r') as f:
    for line in f:
        if line.startswith('username'):
            username = line.split('=')[1].strip()
        if line.startswith('password'):
            password = line.split('=')[1].strip()
        if line.startswith('wallet_password'):
            wallet_pw = line.split('=')[1].strip()

# connect to database
connection=oracledb.connect(
    user=username,
    password=password,
    dsn="cbdcauto_low",
    config_dir="../wallet",
    wallet_location="../wallet",
    wallet_password=wallet_pw
)

print("Autonomous Database Version:", connection.version)

cursor=connection.cursor()
cursor.execute("SELECT * FROM test")
for row in cursor:
    print(row[0], row[1])

# cursor.execute("CREATE TABLE test (id number, name varchar2(20))")
# cursor.execute("INSERT INTO ZEK224.test VALUES (1, 'test')")
# cursor.execute("INSERT INTO test VALUES (2, 'test2')")
# cursor.execute("INSERT INTO test VALUES (3, 'test3')")

# commit
connection.commit()

# close
cursor.close()
connection.close()
