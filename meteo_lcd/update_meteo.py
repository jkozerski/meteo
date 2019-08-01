#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

# Author:
# Janusz Kozerski (https://github.com/jkozerski)

# install:
# pip install python-dateutil
#
# for diagrams install:
# sudo python -m pip --no-cache-dir install -U matplotlib
#
# for database install:
# sqlite3 (apt-get install sqlite3)
#
#
# or for other diagrams (this seems to be too "heavy" for raspberry):
# pip install plotly


import re #regular expression
from shutil import move
from os import remove
from math import sqrt, floor
import datetime # datetime and timedelta structures
import time

# Changing user
import os # os.getuid
import pwd # pwd.getpwuid
import grp # grp.getgrnam

# Mosquito (data passing/sharing)
import paho.mqtt.client as mqtt

# Sqlite3 database
import sqlite3

# Needed for drawing a plot
import dateutil.parser
import matplotlib
import matplotlib.pyplot as plt
#import numpy as np
from matplotlib.ticker import MultipleLocator

##############################################################################################################
### Needed defines & constants

# choose working dir
working_dir = "/var/www/html/"
data_dir    = "/home/pi/meteo/"

www_meteo_path     = working_dir + "meteo.html"
www_meteo_path_tmp = working_dir + "meteo.html_tmp"

log_file_path = working_dir + "meteo.log"
db_path       = data_dir + "meteo.db"

# Default user
default_user = 'pi'

# Diagiam file names
temp_out_diagram_file      = working_dir + "temp_out.png"
humid_out_diagram_file     = working_dir + "humid_out.png"
dew_point_out_diagram_file = working_dir + "dew_out.png"
pressure_diagram_file      = working_dir + "pressure.png"

# We don't want to make update too often so we need to store last update time
# And compare it with current time - if current time is too small then do nothing
# Initialize it to some old time
last_update_time = datetime.datetime.fromtimestamp(1284286794)
last_log_time    = datetime.datetime.fromtimestamp(1284286794)
last_plot_time   = datetime.datetime.fromtimestamp(1284286794)
update_delay = datetime.timedelta(seconds = 60 * 1) # update delay in seconds -> 1 minute
log_delay    = datetime.timedelta(seconds = 60 * 3) # update delay in seconds -> 3 minute
plot_delay   = datetime.timedelta(seconds = 60 * 10) # update delay in seconds -> 1 minute


template_temp_out_begin      = "<!-- TEMP_OUT -->"
template_temp_out_end        = "<!-- /TEMP_OUT -->"
template_humid_out_begin     = "<!-- HUMID_OUT -->"
template_humid_out_end       = "<!-- /HUMID_OUT -->"
template_pressure_begin      = "<!-- PRESS -->"
template_pressure_end        = "<!-- /PRESS -->"
template_dew_point_out_begin = "<!-- DEW_OUT -->"
template_dew_point_out_end   = "<!-- /DEW_OUT -->"
template_temp_in_begin       = "<!-- TEMP_IN -->"
template_temp_in_end         = "<!-- /TEMP_IN -->"
template_humid_in_begin      = "<!-- HUMID_IN -->"
template_humid_in_end        = "<!-- /HUMID_IN -->"
template_dew_point_in_begin  = "<!-- DEW_IN -->"
template_dew_point_in_end    = "<!-- /DEW_IN -->"

template_last_update_begin  = "<!-- LAST_UPDATE -->"
template_last_update_end    = "<!-- /LAST_UPDATE -->"

val_regexp = ".*"

template_temp_out      = template_temp_out_begin      + val_regexp + template_temp_out_end
template_humid_out     = template_humid_out_begin     + val_regexp + template_humid_out_end
template_pressure      = template_pressure_begin      + val_regexp + template_pressure_end
template_dew_point_out = template_dew_point_out_begin + val_regexp + template_dew_point_out_end
template_temp_in       = template_temp_in_begin       + val_regexp + template_temp_in_end
template_humid_in      = template_humid_in_begin      + val_regexp + template_humid_in_end
template_dew_point_in  = template_dew_point_in_begin  + val_regexp + template_dew_point_in_end
template_last_update   = template_last_update_begin   + val_regexp + template_last_update_end


##############################################################################################################
### Change running user to default user ('pi')
# Needed when script is running as root on system startup from /etc/init.d/

def change_user():
    user = pwd.getpwuid( os.getuid() ).pw_name
    if user == default_user :
        print "User OK - '" + user + "'."
        return
    else:
        print "Bad user '" + user + "', changing to '" + default_user + "'."
        try:
            # Remove group privileges
            os.setgroups([])
            # Try setting the new uid/gid
            os.setgid(grp.getgrnam(default_user).gr_gid)
            os.setuid(pwd.getpwnam(default_user).pw_uid)
        except Exception as e:
            print("Error while changing user." + str(e))
    print "User changed from '" + user + "' to '" + default_user + "'."



##############################################################################################################
### Helpers functions

# Returns outside values: temp_out, humid_out, dew_point_out
def get_meteo_data_out():
	return 20.4, 55, 12, 

# Returns inside values: temp_in, humid_in
def get_meteo_data_in():
	return 20.4, 40

# Returns air pressure
def get_meteo_pressure():
	return 999.3

# Calculates dew point
# This should give a correct result for temperature in ranges -30 < T < 70 *C, and humidity  0-100%
def get_dew_point(temp, humid):
	tmp = sqrt(sqrt(sqrt( float(humid)/100.0 ))) * (112.0 + (0.9 * float(temp))) + (0.1 * float(temp)) - 112.0;
	return floor(tmp + 0.5);

# Log data to file
# We can use this data later to draw a plot
def log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure):
	lf = open(log_file_path, "a");
	t = datetime.datetime.now()
	t = t.replace(microsecond=0)
	new_line = t.isoformat() + ";" + str(temp_in) + ";" + str(humid_in) + ";" + str(dew_in) + ";" + str(temp_out) + ";" + str(humid_out) + ";" + str(dew_out) + ";" + str(pressure) + "\n"
	lf.write(new_line)
	lf.close()

# Converts a string back the datetime structure
def getDateTimeFromISO8601String(s):
    d = dateutil.parser.parse(s)
    return d


##############################################################################################################
### Database

# Creating table in database
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


def log_into_db (date_time, temp, humid, dew_point, pressure, temp_in, humid_in, dew_point_in):

    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    try:
        int_time = int (time.mktime(date_time.timetuple()))

        c.execute("INSERT INTO log (time, temp, humid, dew_point, pressure, temp_in, humid_in, dew_point_in) \
                   VALUES (?, ?, ?, ?, ?, ?, ?, ?)", (int_time, temp, humid, dew_point, pressure, temp_in, humid_in, dew_point_in))
        conn.commit()
    except Exception as e:
        print("Error while insert log to database: " + str(e))

    conn.close()


# Get values from last days and hours
def get_val_last_db(days, hours):

    if days < 0 or days > 31:
        return;
    if hours < 0 or hours > 23:
        return;

    current_time = datetime.datetime.now()
    begin_time = current_time - datetime.timedelta(days=days, hours=hours)

    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    c.execute("SELECT strftime('%s', (?))", (begin_time, ))
    int_time_min = (c.fetchone())[0]
    c.execute("SELECT strftime('%s', (?))", (current_time, ))
    int_time_max = (c.fetchone())[0]

    try:
        c.execute("SELECT time, temp, humid, dew_point, pressure FROM log WHERE time >= ? AND time < ?", (int_time_min, int_time_max))
        rows = c.fetchall()

    except Exception as e:
        print("Error while get_val_last from db: " + str(e))

    conn.close()
    return rows


# Get last updatate time from db
def get_last_update_time_from_db():

    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    try:
        c.execute("SELECT time FROM log ORDER BY time DESC LIMIT 1")
        row = c.fetchone()
	ret = row[0]
        print "Last update from db: " + str(ret) + " (" + str(datetime.datetime.fromtimestamp(ret)) + ")"
    except Exception as e:
	print("Cannot read last update time from db: " + str(e))
        ret = 1284286794 # "2010-09-12T12:19:54" - just some random old time

    conn.close()
    return datetime.datetime.fromtimestamp(ret)


##############################################################################################################
### PLOT

def plot_set_ax_fig (time, data, data_len, plot_type, ylabel, title, major_locator, minor_locator, file_name):

    # This keeps chart nice-looking
    ratio = 0.25
    plot_size_inches = 22

    fig, ax = plt.subplots()

    fig.set_size_inches(plot_size_inches, plot_size_inches)

    # Plot data:
    ax.plot_date(time, data, plot_type)
    ax.set_xlim(time[0], time[data_len])
    ax.set(xlabel='', ylabel=ylabel, title=title)
    ax.grid()

    ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=3))
    ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
    ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))

    ax.yaxis.set_major_locator(MultipleLocator(major_locator))
    ax.yaxis.set_minor_locator(MultipleLocator(minor_locator))
    ax.tick_params(labeltop=False, labelright=True)

    plt.gcf().autofmt_xdate()

    xmin, xmax = ax.get_xlim()
    ymin, ymax = ax.get_ylim()
    #print((xmax-xmin)/(ymax-ymin))
    ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio) #, adjustable='box-forced')

    fig.savefig(file_name, bbox_inches='tight')
    plt.close()


# Draw a plot
def draw_plot():
        # Open log file
        lf = open(log_file_path, "r");
        # Calculates number lines in log file
        num_lines = sum(1 for line in lf)
        lf.seek(0)

        # Current time
        current_time = datetime.datetime.now()
        plot_begin_time = current_time - datetime.timedelta(days = 3, hours = 3)

        # Helpers
        j = 0
        values_count = 0
        lines_to_skip = num_lines - 2500
        # This much entries should be more than 3 days and 3 hours of logs.
        # This will cause that generating plot will take less time
        if lines_to_skip < 0:
            lines_to_skip = 0;

	# Use every x (e.g. every second, or every third) value - this makes chart more 'smooth'
        every_x = 1;

        t = []; # time axis for plot
        t_out = []; # temp out for plot
        h_out = []; # humid out for plot
        d_out = []; # dew point for plot
        p_out = []; # pressure for plot
        # From each line of log file create a pairs of meteo data (time, value)
        for line in lf:
            if lines_to_skip > 0:
                lines_to_skip -= 1
                continue
            j += 1
            if j >= every_x:
                j = 0
            else:
                continue
            # Parse line
            time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
            time = getDateTimeFromISO8601String(time)
            if time < plot_begin_time:
                continue;
            values_count += 1
            # Append time for time axis
            t.append(time)
            # Append meteo data for their axis
            t_out.append(float(temp_out))
            h_out.append(float(humid_out))
            d_out.append(float(dew_out))
            p_out.append(float(pressure))

        lf.close()

        # draw plots for outside values: temperature, humidity, dew piont, pressure


	##############
	# Temperature
        plot_set_ax_fig(t, t_out, values_count-1, 'r-', 'Temperatura [C]', 'Wykres temperatury zewnetrznej', 1, 0.1, temp_out_diagram_file)


	##############
	# Humidity
        plot_set_ax_fig(t, h_out, values_count-1, 'g-', 'Wilgotnosc wzgledna [%]', 'Wykres wilgotnosci wzglednej', 5, 1, humid_out_diagram_file)


        ##############
        # Dew point
        plot_set_ax_fig(t, d_out, values_count-1, 'b-', 'Temp. punktu rosy [C]', 'Wykres temperatury punktu rosy', 1, 1, dew_point_out_diagram_file)


        ##############
        # Pressure
        plot_set_ax_fig(t, p_out, values_count-1, 'm-', 'Cisnienie atm. [hPa]', 'Wykres cisnienia atmosferycznego', 2, 1, pressure_diagram_file)

        return


# Draw a plot using database
def draw_plot_db():

        t = []; # time axis for plot
        t_out = []; # temp out for plot
        h_out = []; # humid out for plot
        d_out = []; # dew point for plot
        p_out = []; # pressure for plot

        rows = get_val_last_db(3, 3)  # 3 days, 3 hours
        # From each row creates a pairs of meteo data (time, value)

	values_count = len(rows)
        # Row format: (time, temp, humid, dew_point, pressure)
        for row in rows:
            # Append time for time axis
            t.append(datetime.datetime.fromtimestamp(row[0]))
            # Append meteo data for their axis
            t_out.append(row[1])
            h_out.append(row[2])
            d_out.append(row[3])
            p_out.append(row[4])

        # draw plots for outside values: temperature, humidity, dew piont, pressure

	##############
	# Temperature
        plot_set_ax_fig(t, t_out, values_count-1, 'r-', 'Temperatura [C]', 'Wykres temperatury zewnetrznej', 1, 0.1, temp_out_diagram_file)


	##############
	# Humidity
        plot_set_ax_fig(t, h_out, values_count-1, 'g-', 'Wilgotnosc wzgledna [%]', 'Wykres wilgotnosci wzglednej', 5, 1, humid_out_diagram_file)


        ##############
        # Dew point
        plot_set_ax_fig(t, d_out, values_count-1, 'b-', 'Temp. punktu rosy [C]', 'Wykres temperatury punktu rosy', 1, 1, dew_point_out_diagram_file)


        ##############
        # Pressure
        plot_set_ax_fig(t, p_out, values_count-1, 'm-', 'Cisnienie atm. [hPa]', 'Wykres cisnienia atmosferycznego', 2, 1, pressure_diagram_file)

        return


# Get last updatate time from log file
def get_last_update_time_from_log():
        # Open log file
	lf = open(log_file_path, "r");
	for line in lf:
	    pass
	last = line
	lf.close()
	try:
		time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
		print ("Last update time (from log) set to: " + time)
	        return getDateTimeFromISO8601String(time)
	except Exception as e:
		print("Cannot read last update time from log: " + str(e))
	return datetime.datetime.fromtimestamp(1284286794)


##############################################################################################################
### Main update function
### Updates: webpage, text file log, database, and plot

# Update web data:
# Meteo data comes to function as a parameters in order:
# temp in, humid in, temp_out, humid out, pressure
def update_meteo_data(data):

        # Meteo data comes to function as a parameters in order:
        # temp in, humid in, temp_out, humid out, pressure
        try:
		temp_in, humid_in, temp_out, humid_out, pressure = data.split(";")
		val = float(temp_in)
		val = int(humid_in)
		val = float(temp_out)
		val = int(humid_out)
		val = float(pressure)
        except Exception as e:
		print("Bad meteo data values: " + str(e))
		return
        pressure = float(pressure) / 100.0

	# Get current time
	current_time = datetime.datetime.now()
	# Reset microsecond in current time to 0 - we don't want to keep them
	current_time = current_time.replace(microsecond=0)
        print(current_time)
	
	global last_update_time
	global last_log_time
	global last_plot_time
	# if now() - last_update_time < update_delay - then do nothing
	if current_time - last_update_time < update_delay:
		# do nothing
		#print("No need to update")
		return
	
	print("Web update")
	# save last update time
	last_update_time = current_time;
	
	# Open html template file (web page) 
	tmp_file = open(www_meteo_path_tmp, "r")
	# Remove old file
	try:
	        remove(www_meteo_path)
	except Exception as e:
		print("Skip removing file: " + str(e))
	# Open (create) new html file
	new_file = open(www_meteo_path, "w")

	# Meteo data comes to function as a parameters in order:
	# temp in, humid in, temp_out, humid out, pressure
	#temp_in, humid_in, temp_out, humid_out, pressure = data.split(";")

	# Calculate dew point (in and out)
	dew_out = get_dew_point(temp_out, humid_out);
	dew_in  = get_dew_point(temp_in, humid_in);
	
	# In html file replace old data with new data using regular expressions an templates
	for line in tmp_file:
		# out:
		line = line.decode("utf-8") # Remove this for Python 3.x
		new_line = re.sub(template_temp_out,      template_temp_out_begin      + str(temp_out)  + template_temp_out_end,      line)
		new_line = re.sub(template_humid_out,     template_humid_out_begin     + str(humid_out) + template_humid_out_end,     new_line)
		new_line = re.sub(template_dew_point_out, template_dew_point_out_begin + str(dew_out)   + template_dew_point_out_end, new_line)
		# pressure:
		new_line = re.sub(template_pressure,      template_pressure_begin      + str(pressure)  + template_pressure_end,      new_line)
		# in:
		new_line = re.sub(template_temp_in,       template_temp_in_begin       + str(temp_in)   + template_temp_in_end,       new_line)
		new_line = re.sub(template_humid_in,      template_humid_in_begin      + str(humid_in)  + template_humid_in_end,      new_line)
		new_line = re.sub(template_dew_point_in,  template_dew_point_in_begin  + str(dew_in)    + template_dew_point_in_end,  new_line)
		# last update:
		new_line = re.sub(template_last_update,   template_last_update_begin + current_time.isoformat(' ') + template_last_update_end, new_line)
		# write to file:
		new_file.write(new_line.encode("utf-8"))
		#new_file.write(new_line) # use this for Python 3.x
	
	# Close opened files
	tmp_file.close()
	new_file.close()

	# Save data in log file (we can use to draw a plot)
	if current_time >= last_log_time + log_delay:
		last_log_time = current_time
		# put data into log file
                print("Update log")
                log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure)
                log_into_db (current_time, temp_out, humid_out, dew_out, pressure, temp_in, humid_in, dew_in)

	# Drawing a plot
	if current_time >= last_plot_time + plot_delay:
                last_plot_time = current_time
		start = time.time()
		print("Draw a plot using db")
		draw_plot_db()
		end = time.time()
		print("Drawing done! (in " + str(int(end - start)) + "s)")


##############################################################################################################
### Main program
##############################################################################################################
# Based on:
# Thomas Varnish (https://github.com/tvarnish), (https://www.instructables.com/member/Tango172)
# Written for my Instructable - "How to use MQTT with the Raspberry Pi and ESP8266"

change_user()

# Creating db - creates it only if it doesn't exist
create_db()
print "Database OK"

# MQTT init
mqtt_username = "meteo"
mqtt_password = "meteo1234"
mqtt_topic = "meteo"
mqtt_broker_ip = "192.168.0.8"

client = mqtt.Client()
# Set the username and password for the MQTT client
client.username_pw_set(mqtt_username, mqtt_password)

# Event handlers
def on_connect(client, self, userdata, rc):
    # rc is the error code returned when connecting to the broker
    print "Connected!", str(rc)

    # Once the client has connected to the broker, subscribe to the topic
    client.subscribe(mqtt_topic, 0)

def on_message(client, userdata, msg):
    #print "\n----------------\nTopic: ", msg.topic + "\nMessage: " + str(msg.payload)
    print "\nMessage: " + str(msg.payload)
    update_meteo_data(msg.payload);

# Check last update and log time from log file
last_update_time = get_last_update_time_from_db()
#last_update_time = get_last_update_time_from_log()
last_log_time    = last_update_time
last_plot_time   = last_update_time

client.on_connect = on_connect
client.on_message = on_message

while True:
    try:
        print("Try to connect to MQTT broker.")
        client.connect(mqtt_broker_ip, 1883)
    except Exception as e:
        print("MQTT client connect failed: " + str(e))
        time.sleep(5);
        continue;
    break;
print("MQTT client connected")

# Once we have told the client to connect, let the client object run itself
client.loop_forever()
client.disconnect()
