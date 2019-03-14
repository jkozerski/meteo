import sqlite3
import datetime
import dateutil.parser

working_dir = "/var/www/html/"
data_dir = "/home/pi/meteo/"

log_file_path = working_dir + "meteo.log"
db_path = data_dir + "meteo.db"

###################################

def db_to_log():

    lf = open(data_dir + "tmp.log", "w");

    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    try:
        c.execute("SELECT time, temp_in, humid_in, dew_point_in, temp, humid, dew_point, pressure FROM log ORDER BY time ASC")
        rows = c.fetchall()
    except Exception as e:
        print("Error while get all values from db: " + str(e))

    for row in rows:
        new_line = datetime.datetime.fromtimestamp(row[0]).isoformat() + ";" + \
                   str(row[1]) + ";" + \
                   str(row[2]) + ";" + \
                   str(float(row[3])) + ";" + \
                   str(row[4]) + ";" + \
                   str(row[5]) + ";" + \
                   str(float(row[6])) + ";" + \
                   str(row[7]) + "\n"
        lf.write(new_line)

    lf.close()
    conn.close()


###################################
db_to_log()

