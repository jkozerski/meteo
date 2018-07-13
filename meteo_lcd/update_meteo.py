# -*- coding: utf-8 -*-

# install:
# pip install python-dateutil
# pip install plotly

import re #regular expression
from shutil import move
from os import remove
from math import sqrt, floor
import datetime # datetime and timedelta structures


# Needed for drawing a plot
import dateutil.parser
import plotly
#import plotly.plotly as py
#import plotly.graph_objs as go

# for testing purpose
import random
import time

# choose working dir
#working_dir = "/var/www/html/"
working_dir = "/home/januszk/workspace/priv/meteo/meteo_lcd/"

www_meteo_path     = working_dir + "meteo.html"
www_meteo_path_tmp = working_dir + "meteo.html_tmp"

log_file_path = working_dir + "meteo.log"

# Diagiam file names
temp_out_diagram_file      = "temp_out.html"
humid_out_diagram_file     = "humid_out.html"
dew_point_out_diagram_file = "dew_out.html"
pressure_diagram_file      = "pressure.html"

# We don't want to make update too often so we need to store last update time
# And compare it with current time - if current time is too small then do nothing
# Initialize it to some old time
last_update_time = datetime.datetime.fromtimestamp(1284286794)
update_delay = datetime.timedelta(seconds = 6 * 5) # update delay in seconds -> 5 minutes


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
	t = []; # time axis for plot
	t_out = []; # temp out for plot
	h_out = []; # humid out for plot
	d_out = []; # dew point for plot
	p_out = []; # pressure for plot
	# From each line of log file create a pairs of meteo data (time, value)
	for line in lf:
		# Parse line
		time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
		# Append time for time axis
		t.append(getDateTimeFromISO8601String(time))
		# Append meteo data for their axis
		t_out.append(float(temp_out))
		h_out.append(float(humid_out))
		d_out.append(float(dew_out))
		p_out.append(float(pressure))
	lf.close()
	# Create datasets
	data_temp  = [plotly.graph_objs.Scatter(x=t, y=t_out)]
	data_humid = [plotly.graph_objs.Scatter(x=t, y=h_out)]
	data_dew   = [plotly.graph_objs.Scatter(x=t, y=d_out)]
	data_press = [plotly.graph_objs.Scatter(x=t, y=p_out)]
	
	# draw plots for outside values: temperature, humidity, dew piont, pressure
	generate_plot(data_temp,  temp_out_diagram_file)
	generate_plot(data_humid, humid_out_diagram_file)
	generate_plot(data_dew,   dew_point_out_diagram_file)
	generate_plot(data_press, pressure_diagram_file)
	
	return


# Update web data:
# Meteo data comes to function as a parameters in order:
# temp in, humid in, temp_out, humid out, pressure
def update_meteo_data(data):
	# Get current time
	last_update = datetime.datetime.now()
	# Reset microsecond in current time to 0 - we don't want to keep them
	last_update = last_update.replace(microsecond=0)
	
	global last_update_time
	# if now() - last_update_time < update_delay - then do nothing
	if last_update - last_update_time < update_delay:
		# do nothing
		print("No need to update")
		return
	
	print("Need to update")
	last_update_time = last_update;
	
	# Open html (web page) file with meteo data
	old_file = open(www_meteo_path, "r")
	# Open temporary html file
	new_file = open(www_meteo_path_tmp, "w")

	# Meteo data comes to function as a parameters in order:
	# temp in, humid in, temp_out, humid out, pressure
	temp_in, humid_in, temp_out, humid_out, pressure = data.split(";")

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
	log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure);
	# Draw a plot
	draw_plot()

# Main program
update_meteo_data("30.0; 60; " + str(random.randint(-20, 30)) + "; 90; 1013.3");

