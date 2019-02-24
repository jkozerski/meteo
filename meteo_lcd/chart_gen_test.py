import matplotlib
import matplotlib.pyplot as plt
import numpy as np

import re #regular expression
from shutil import move
from os import remove
from math import sqrt, floor
import datetime # datetime and timedelta structures

from matplotlib.ticker import MultipleLocator

# Mosquito (data passing/sharing)
#import paho.mqtt.client as mqtt

# Needed for drawing a plot
import dateutil.parser
#import plotly

# Data for plotting
#t = np.arange(0.0, 2.0, 1)
#s = 1 + np.sin(2 * np.pi * t)

#fig, ax = plt.subplots()
#ax.plot(t, s)

#ax.set(xlabel='time (s)', ylabel='voltage (mV)',
#       title='About as simple as it gets, folks')
#ax.grid()

#fig.savefig("test.png")
#plt.show()

def getDateTimeFromISO8601String(s):
    d = dateutil.parser.parse(s)
    return d

def draw_plot():
        # Open log file
        lf = open("C:\Users\Janusz\meteo.log", "r")
        num_lines = sum(1 for line in lf)
        lf.seek(0)
        print(num_lines)
        lines_to_skip = num_lines - 350 # 864 - aprox. 3 full days
        i = 0;
        j = 0;
        every_x = 1;
        x_pixels = 700
        y_pixels = 400
        minorLocator_y = MultipleLocator(0.1)
        majorLocator_y = MultipleLocator(1)
        minorLocator_t = MultipleLocator(0.01)
        
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
        fig, ax = plt.subplots()
        #plt.figure(figsize=(y_pixels/100.0, x_pixels/100.0), dpi=100)
        ax.set_aspect(0.05)
        fig.set_size_inches(16, 8)
        #ax.plot(t, t_out)
        ax.plot_date(t, t_out, 'b-')

        ax.set(xlabel='Czas', ylabel='Temperatura zew. [C] ',
               title='About as simple as it gets, folks')
        ax.grid()

        ax.yaxis.set_minor_locator(minorLocator_y)
        ax.yaxis.set_major_locator(majorLocator_y)
        ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=None, interval=2))
        ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator())
        ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))
        #ax.yaxis.set_major_formatter(majorFormatter)

        plt.gcf().autofmt_xdate()
        
        #fig.savefig("test.png", dpi=200)
        fig.savefig("test.png", bbox_inches='tight') 
        plt.show()
        #plt.close()

        #generate_plot(data_temp,  temp_out_diagram_file)
        #generate_plot(data_humid, humid_out_diagram_file)
        #generate_plot(data_dew,   dew_point_out_diagram_file)
        #generate_plot(data_press, pressure_diagram_file)

        return

draw_plot()
