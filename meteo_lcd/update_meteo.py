#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

# install:
# pip install python-dateutil
# for diagrams (this seems to be too "heavy for raspberry"):
# pip install plotly

# or for other diagrams:
# sudo python -m pip --no-cache-dir install -U matplotlib


import re #regular expression
from shutil import move
from os import remove
from math import sqrt, floor
import datetime # datetime and timedelta structures

# Mosquito (data passing/sharing)
import paho.mqtt.client as mqtt

# Needed for drawing a plot
import dateutil.parser
#import plotly
#import plotly.plotly as py
#import plotly.graph_objs as go
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MultipleLocator

# for testing purpose
import random
import time

# choose working dir
working_dir = "/var/www/html/"
#working_dir = "/home/januszk/workspace/priv/meteo/meteo_lcd/"

www_meteo_path     = working_dir + "meteo.html"
www_meteo_path_tmp = working_dir + "meteo.html_tmp"

log_file_path = working_dir + "meteo.log"

# Diagiam file names
temp_out_diagram_file      = working_dir + "temp_out.png"
humid_out_diagram_file     = working_dir + "humid_out.png"
dew_point_out_diagram_file = working_dir + "dew_out.png"
pressure_diagram_file      = working_dir + "pressure.png"

# We don't want to make update too often so we need to store last update time
# And compare it with current time - if current time is too small then do nothing
# Initialize it to some old time
last_update_time = datetime.datetime.fromtimestamp(1284286794)
update_delay = datetime.timedelta(seconds = 60 * 1) # update delay in seconds -> 1 minute
log_delay = 5 # update log every 5 webpage update (~5 min)
log_delay_iter = 0 # iterator initialization


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

# draw a plot from the data into a html file
def generate_plot(data, filename):
	#plotly.offline.plot(data, show_link=True, link_text='Export to plot.ly', validate=True, output_type='file', include_plotlyjs=True, filename='temp_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	plotly.offline.plot(data, show_link=False, link_text='Export to plot.ly', validate=False, output_type='file', include_plotlyjs=True, filename=working_dir+filename, auto_open=False, image=None, image_filename='plot_image', image_width=600, image_height=800, config=None)

# Draw a plot
def draw_plot():
	# Open log file
	lf = open(log_file_path, "r");
	# Calculates number lines in log file
        num_lines = sum(1 for line in lf)
        lf.seek(0)

	# We want to use only a few last lines (chart of last few hours/days)
        values_count = 800 # 800 - aprox. 3 full days
        lines_to_skip = num_lines - values_count

	# This keeps chart nice-looking
	ratio = 0.25;

	# Helpers
        i = 0;
        j = 0;

	# Use every x (e.g. every second, or every third) value - this makes chart more 'smooth'
        every_x = 1;

        t = []; # time axis for plot
        t_out = []; # temp out for plot
        h_out = []; # humid out for plot
        d_out = []; # dew point for plot
        p_out = []; # pressure for plot
        # From each line of log file create a pairs of meteo data (time, value)
        for line in lf:
            i += 1
            j += 1
            if i < lines_to_skip:
                continue
            if j >= every_x:
                j = 0
            else:
                continue
            # Parse line
            time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
            # Append time for time axis
            #t.append(matplotlib.dates.date2num(getDateTimeFromISO8601String(time)))
            t.append(getDateTimeFromISO8601String(time))
            # Append meteo data for their axis
            t_out.append(float(temp_out))
            h_out.append(float(humid_out))
            d_out.append(float(dew_out))
            p_out.append(float(pressure))

        lf.close()

        # draw plots for outside values: temperature, humidity, dew piont, pressure


	##############
	# Temperature
        fig, ax = plt.subplots()
        #ax.set_aspect(0.05)
        #ax.set_aspect(0.04)
        fig.set_size_inches(20, 20)
        ax.plot_date(t, t_out, 'b-')

	ax.set_xlim(t[0], t[values_count-1])
        ax.set(xlabel='', ylabel='Temperatura [C] ',
               title='Wykres temperatury zewnetrznej')
        ax.grid()

        ax.yaxis.set_minor_locator(MultipleLocator(0.1))
        ax.yaxis.set_major_locator(MultipleLocator(1))
        ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=3))
        ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
        ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))
        #ax.yaxis.set_major_formatter(majorFormatter)
        ax.tick_params(labeltop=False, labelright=True)

        plt.gcf().autofmt_xdate()

        xmin, xmax = ax.get_xlim()
        ymin, ymax = ax.get_ylim()
        #print((xmax-xmin)/(ymax-ymin))
        ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio) #, adjustable='box-forced')

        #fig.savefig("test.png", dpi=200)
        fig.savefig(temp_out_diagram_file, bbox_inches='tight')
        plt.close()


	##############
	# Humidity
        fig, ax = plt.subplots()
        ax.set_aspect(0.04)
        fig.set_size_inches(20, 20)
	ax.plot_date(t, h_out, 'b-')

	ax.set_xlim(t[0], t[values_count-1])
        ax.set(xlabel='', ylabel='Wilgotnosc wzgledna [%] ',
               title='Wykres wilgotnosci wzglednej')
        ax.grid()

        ax.yaxis.set_minor_locator(MultipleLocator(1))
        ax.yaxis.set_major_locator(MultipleLocator(5))
        ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=3))
        ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
        ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))
        ax.tick_params(labeltop=False, labelright=True)

        plt.gcf().autofmt_xdate()

        xmin, xmax = ax.get_xlim()
        ymin, ymax = ax.get_ylim()
        ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio)

        fig.savefig(humid_out_diagram_file, bbox_inches='tight')
        plt.close()


	##############
	# Dew point
        fig, ax = plt.subplots()
        ax.set_aspect(0.04)
        fig.set_size_inches(20, 20)
	ax.plot_date(t, d_out, 'b-')

	ax.set_xlim(t[0], t[values_count-1])
        ax.set(xlabel='', ylabel='Temp. punktu rosy [C] ',
               title='Wykres temperatury punktu rosy')
        ax.grid()

        ax.yaxis.set_minor_locator(MultipleLocator(1))
        ax.yaxis.set_major_locator(MultipleLocator(1))
        ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=3))
        ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
        ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))
        ax.tick_params(labeltop=False, labelright=True)

        plt.gcf().autofmt_xdate()

        xmin, xmax = ax.get_xlim()
        ymin, ymax = ax.get_ylim()
        ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio)

        fig.savefig(dew_point_out_diagram_file, bbox_inches='tight')
        plt.close()


	##############
	# Pressure
        fig, ax = plt.subplots()
        ax.set_aspect(0.04)
        fig.set_size_inches(20, 20)
	ax.plot_date(t, p_out, 'b-')

	ax.set_xlim(t[0], t[values_count-1])
        ax.set(xlabel='', ylabel='Cisnienie atm. [hPa]',
               title='Wykres cisnienia atmosferycznego')
        ax.grid()

        ax.yaxis.set_minor_locator(MultipleLocator(1))
        ax.yaxis.set_major_locator(MultipleLocator(2))
        ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=3))
        ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
        ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))
        ax.tick_params(labeltop=False, labelright=True)

        plt.gcf().autofmt_xdate()

        xmin, xmax = ax.get_xlim()
        ymin, ymax = ax.get_ylim()
        ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio)

        fig.savefig(pressure_diagram_file, bbox_inches='tight')
        plt.close()

        return


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
	last_update = datetime.datetime.now()
	# Reset microsecond in current time to 0 - we don't want to keep them
	last_update = last_update.replace(microsecond=0)
	
	global last_update_time
	global log_delay_iter
	# if now() - last_update_time < update_delay - then do nothing
	if last_update - last_update_time < update_delay:
		# do nothing
		#print("No need to update")
		return
	
	print("Web update")
	# save last update time
	last_update_time = last_update;
	
	# Open html (web page) file with meteo data
	old_file = open(www_meteo_path, "r")
	# Open temporary html file
	new_file = open(www_meteo_path_tmp, "w")

	# Meteo data comes to function as a parameters in order:
	# temp in, humid in, temp_out, humid out, pressure
	#temp_in, humid_in, temp_out, humid_out, pressure = data.split(";")

	# Calculate dew point (in and out)
	dew_out = get_dew_point(temp_out, humid_out);
	dew_in  = get_dew_point(temp_in, humid_in);
	
	# In html file replace old data with new data using regular expressions an templates
	for line in old_file:
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
		new_line = re.sub(template_last_update,   template_last_update_begin + last_update.isoformat(' ') + template_last_update_end, new_line)
		# write to file:
		new_file.write(new_line.encode("utf-8"))
		#new_file.write(new_line) # use this for Python 3.x
	
	# Close opened files
	old_file.close()
	new_file.close()
	# Remove old file
	remove(www_meteo_path)
	# Move new file (with new data) instead of old one
	move(www_meteo_path_tmp, www_meteo_path)

	# Save data in log file (we can use to draw a plot)
	if log_delay_iter <= 0:
                log_delay_iter = log_delay;
		# put data into log file
		print("Update log")
		log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure);
		print("Draw a plot")
		draw_plot()
		print("Drawing done!")
        log_delay_iter -= 1;


	# Draw a plot
        # do not draw it - generating and displaying it costs too much CPU
	# draw_plot()

# Main program
#update_meteo_data("30.0; 60; " + str(random.randint(-20, 30)) + "; 90; 1013.3");

# Based on:
# Thomas Varnish (https://github.com/tvarnish), (https://www.instructables.com/member/Tango172)
# Written for my Instructable - "How to use MQTT with the Raspberry Pi and ESP8266"


mqtt_username = "meteo"
mqtt_password = "meteo1234"
mqtt_topic = "meteo"
#mqtt_broker_ip = "meteo"
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


client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_broker_ip, 1883)

# Once we have told the client to connect, let the client object run itself
client.loop_forever()
client.disconnect()
