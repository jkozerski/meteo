# -*- coding: utf-8 -*-

import re
from shutil import move
from os import remove
from math import sqrt, floor


#www_meteo_path     = "/var/www/html/index.html"
#www_meteo_path_tmp = "/var/www/html/index.html_tmp"

www_meteo_path     = "/home/januszk/workspace/priv/meteo/meteo_lcd/meteo.html"
www_meteo_path_tmp = "/home/januszk/workspace/priv/meteo/meteo_lcd/meteo.html_tmp"

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

val_regexp = ".*"

template_temp_out      = template_temp_out_begin      + val_regexp + template_temp_out_end
template_humid_out     = template_humid_out_begin     + val_regexp + template_humid_out_end
template_pressure      = template_pressure_begin      + val_regexp + template_pressure_end
template_dew_point_out = template_dew_point_out_begin + val_regexp + template_dew_point_out_end
template_temp_in       = template_temp_in_begin       + val_regexp + template_temp_in_end
template_humid_in      = template_humid_in_begin      + val_regexp + template_humid_in_end
template_dew_point_in  = template_dew_point_in_begin  + val_regexp + template_dew_point_in_end

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
		# write to file:
		new_file.write(new_line)
	
	# Remove old file
	remove(www_meteo_path)
	# Move new file
	move(www_meteo_path_tmp, www_meteo_path)

# Main program
update_meteo_data("21.0; 50; -20.5; 90; 1013.3");

