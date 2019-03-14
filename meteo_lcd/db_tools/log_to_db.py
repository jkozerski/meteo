import sqlite3
import datetime
import dateutil.parser

working_dir = "/var/www/html/"
data_dir = "/home/pi/meteo/"

log_file_path = working_dir + "meteo.log"
db_path = data_dir + "meteo.db"


# Converts a string back the datetime structure
def getDateTimeFromISO8601String(s):
    d = dateutil.parser.parse(s)
    return d


def log_to_db ():
    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    # Open log file
    lf = open(log_file_path, "r");

    # From each line of log file create a pairs of meteo data (time, value)
    for line in lf:
        # Parse line
        time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
        #c.execute("SELECT strftime('%s', (?), 'localtime')", (time, ))
        #int_time = (c.fetchone())[0]
        int_time = int (getDateTimeFromISO8601String(time).strftime("%s"))
        #time_ = getDateTimeFromISO8601String(time)
        #print int_time
        c.execute("INSERT INTO log (time, temp, humid, dew_point, pressure, temp_in, humid_in, dew_point_in) \
                   VALUES (?, ?, ?, ?, ?, ?, ?, ?)", (int_time, temp_out, humid_out, dew_out, pressure, temp_in, humid_in, dew_in))

    lf.close()
    conn.commit()
    conn.close()


def create_db ():
    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    sql = "CREATE TABLE IF NOT EXISTS log (\n\
id INTEGER PRIMARY KEY ASC,\n\
time INT NOT NULL,\n\
temp REAL,\n\
humid INT,\n\
dew_point INT,\n\
pressure REAL,\n\
temp_in REAL,\n\
humid_in INT,\n\
dew_point_in INT)"
                             
    c.execute(sql)

    conn.commit()
    conn.close()

print ("Creating database..")
create_db()
print ("Database OK")
print ("Copying from log file to database...")
log_to_db()
print ("Copy OK")

