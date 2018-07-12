# -*- coding: utf-8 -*-

# install:
# pip install python-dateutil
# pip install plotly

import re #regular expression
from shutil import move
from os import remove
from math import sqrt, floor
from datetime import datetime


# Needed for drawing a plot
import dateutil.parser
import plotly
#import plotly.plotly as py
#import plotly.graph_objs as go

# for testing purpose
import random


#www_meteo_path     = "/var/www/html/index.html"
#www_meteo_path_tmp = "/var/www/html/index.html_tmp"

www_meteo_path     = "/home/januszk/workspace/priv/meteo/meteo_lcd/meteo.html"
www_meteo_path_tmp = "/home/januszk/workspace/priv/meteo/meteo_lcd/meteo.html_tmp"


#log_file_path = "/var/www/html/meteo.log"
log_file_path = "/home/januszk/workspace/priv/meteo/meteo_lcd/meteo.log"


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
def get_dew_point(temp, humid):
	tmp = sqrt(sqrt(sqrt( float(humid)/100.0 ))) * (112.0 + (0.9 * float(temp))) + (0.1 * float(temp)) - 112.0;
	return floor(tmp + 0.5);

# Log data to file
# We can use this data later to draw a plot
def log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure):
	lf = open(log_file_path, "a");
	t = datetime.now()
	t = t.replace(microsecond=0)
	new_line = t.isoformat() + ";" + str(temp_in) + ";" + str(humid_in) + ";" + str(dew_in) + ";" + str(temp_out) + ";" + str(humid_out) + ";" + str(dew_out) + ";" + str(pressure) + "\n"
	lf.write(new_line)
	lf.close()

# Converts a string back the datetime structure
def getDateTimeFromISO8601String(s):
    d = dateutil.parser.parse(s)
    return d

# Draw a plot
def draw_plot():
	lf = open(log_file_path, "r");
	t = []; # time for plot
	t_out = []; # temp out for plot
	h_out = []; # humid out for plot
	d_out = []; # dew point for plot
	p_out = []; # pressure for plot
	for line in lf:
		time, temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure = str(line).split(";")
		t.append(getDateTimeFromISO8601String(time))
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
	
	#plotly.offline.plot(data, show_link=True, link_text='Export to plot.ly', validate=True, output_type='file', include_plotlyjs=True, filename='temp_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	
	# draw temp out plot into html file
	plotly.offline.plot(data_temp, show_link=False, link_text='Export to plot.ly', validate=False, output_type='file', include_plotlyjs=True, filename='temp_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	# draw humid out plot into html file
	plotly.offline.plot(data_humid, show_link=False, link_text='Export to plot.ly', validate=False, output_type='file', include_plotlyjs=True, filename='humid_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	# draw dew point out plot into html file
	plotly.offline.plot(data_humid, show_link=False, link_text='Export to plot.ly', validate=False, output_type='file', include_plotlyjs=True, filename='dew_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	# draw pressure plot into html file
	plotly.offline.plot(data_humid, show_link=False, link_text='Export to plot.ly', validate=False, output_type='file', include_plotlyjs=True, filename='press_out.html', auto_open=False, image=None, image_filename='plot_image', image_width=800, image_height=600, config=None)
	return


# Update web data:
def update_meteo_data(data):
	old_file = open(www_meteo_path, "r")
	new_file = open(www_meteo_path_tmp, "w")

	#temp_out, humid_out, dew_out = get_meteo_data_out()
	#temp_in, humid_in = get_meteo_data_in()
	#pressure = get_meteo_pressure()
	temp_in, humid_in, temp_out, humid_out, pressure = data.split(";")
	dew_out = get_dew_point(temp_out, humid_out);
	dew_in  = get_dew_point(temp_in, humid_in);
	last_update = datetime.now()
	last_update = last_update.replace(microsecond=0)
	
	for line in old_file:
		# out:
		new_line = re.sub(template_temp_out,      template_temp_out_begin      + str(temp_out)  + template_temp_out_end,      line)
		new_line = re.sub(template_humid_out,     template_humid_out_begin     + str(humid_out) + template_humid_out_end,     new_line)
		new_line = re.sub(template_dew_point_out, template_dew_point_out_begin + str(dew_out)   + template_dew_point_out_end, new_line)
		# pressure:
		new_line = re.sub(template_pressure,      template_pressure_begin      + str(pressure)  + template_pressure_end,      new_line)
		# in:
		new_line = re.sub(template_temp_in,       template_temp_in_begin       + str(temp_in)   + template_temp_in_end,       new_line)
		new_line = re.sub(template_humid_in,      template_humid_in_begin      + str(humid_in)  + template_humid_in_end,      new_line)
		new_line = re.sub(template_dew_point_in,  template_dew_point_in_begin  + str(dew_in)    + template_dew_point_in_end,  new_line)
		# las update:
		new_line = re.sub(template_last_update,   template_last_update_begin + last_update.isoformat(' ') + template_last_update_end,   new_line)
		# write to file:
		new_file.write(new_line)
	
	old_file.close()
	new_file.close()
	# Remove old file
	remove(www_meteo_path)
	# Move new file
	move(www_meteo_path_tmp, www_meteo_path)
	# Save data in log file
	log_to_file(temp_in, humid_in, dew_in, temp_out, humid_out, dew_out, pressure);
	# Draw a plot
	draw_plot()

# Main program
update_meteo_data("30.0; 60; " + str(random.randint(-20, 30)) + "; 90; 1013.3");

