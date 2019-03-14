#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

# Author:
# Janusz Kozerski (https://github.com/jkozerski)

# This draws plots (of temp, humid, dp and pressure) for prevoius month.
# Plots are kept in files:
# yyyy.mm.dataName.png

import dateutil.parser
import datetime # datetime and timedelta structures
import time
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

# Sqlite3 database
import sqlite3


# choose working dir
working_dir = "/var/www/html/"
data_dir    = "/home/pi/meteo/"
hist_dir    = working_dir + "hist/"

log_file_path = working_dir + "meteo.log"
db_path       = data_dir + "meteo.db"

# Diagiam file names
temp_out_diagram_file      = "temp_out.png"
humid_out_diagram_file     = "humid_out.png"
dew_point_out_diagram_file = "dew_out.png"
pressure_diagram_file      = "pressure.png"


# Converts a string back the datetime structure
def getDateTimeFromISO8601String(s):
    d = dateutil.parser.parse(s)
    return d


def get_val_month_db(month, year):

    if month < 1 or month > 12:
        return;
    if year < 2000 or year > 9999:
        return;

    conn = sqlite3.connect(db_path)
    c = conn.cursor()

    str_time_min = str(year).zfill(4) + "-" + str(month).zfill(2)   + "-01T00:00:00"
    str_time_max = str(year).zfill(4) + "-" + str(month+1).zfill(2) + "-01T00:00:00"

    #c.execute("SELECT strftime('%s', (?))", (str_time_min, ))
    #int_time_min = (c.fetchone())[0]
    #c.execute("SELECT strftime('%s', (?))", (str_time_max, ))
    #int_time_max = (c.fetchone())[0]

    int_time_min = int (time.mktime(getDateTimeFromISO8601String(str_time_min).timetuple()))
    int_time_max = int (time.mktime(getDateTimeFromISO8601String(str_time_max).timetuple()))

    try:
        c.execute("SELECT time, temp, humid, dew_point, pressure FROM log WHERE time >= ? AND time < ?", (int_time_min, int_time_max))
        rows = c.fetchall()
#        for row in rows:
#            print(row)

    except Exception as e:
        print("Error while get_val_month from db: " + str(e))

    conn.close()
    return rows




def plot_set_ax_fig (today, time, data, data_len, plot_type, ylabel, title, major_locator, minor_locator, file_name):

    # This keeps chart nice-looking
    ratio = 0.25
    plot_size_inches = 28

    fig, ax = plt.subplots()

    fig.set_size_inches(plot_size_inches, plot_size_inches)

    # Plot data:
    ax.plot_date(time, data, plot_type)
    ax.set_xlim(time[0], time[data_len])
    ax.set(xlabel='', ylabel=ylabel, title=title + " " + str(today.month-1) + "." + str(today.year))
    ax.grid()

    ax.xaxis.set_major_locator(matplotlib.dates.HourLocator(byhour=(0)))
    ax.xaxis.set_minor_locator(matplotlib.dates.HourLocator(byhour=(0,6,12,18)))
    ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%m-%d %H:%M"))

    ax.yaxis.set_major_locator(MultipleLocator(major_locator))
    ax.yaxis.set_minor_locator(MultipleLocator(minor_locator))
    ax.tick_params(labeltop=False, labelright=True)

    plt.gcf().autofmt_xdate()

    xmin, xmax = ax.get_xlim()
    ymin, ymax = ax.get_ylim()
    #print((xmax-xmin)/(ymax-ymin))
    ax.set_aspect(abs((xmax-xmin)/(ymax-ymin))*ratio) #, adjustable='box-forced')

    fig.savefig(hist_dir + str(today.year) + "." + str(today.month-1) + "." + file_name, bbox_inches='tight')
    plt.close()

# Draw a plot
def draw_plot_month():
    # Open log file
    lf = open(log_file_path, "r");
    # Calculates number lines in log file
    num_lines = sum(1 for line in lf)
    lf.seek(0)

    # This keeps chart nice-looking
    ratio = 0.25
    plot_size_inches = 28

    # Today
    today = datetime.datetime.today()
    plot_date_begin = datetime.datetime(today.year, today.month-1, 1)
    plot_date_end   = datetime.datetime(today.year, today.month, 1)

    # Helpers
    j = 0
    values_count = 0
    lines_to_skip = num_lines - 25000
    # This much entries should be more than one month (31 days)
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
        if time < plot_date_begin:
            continue
        if time > plot_date_end:
            continue
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
    plot_set_ax_fig(today, t, t_out, values_count-1, 'r-', 'Temperatura [C]', 'Wykres temperatury zewnetrznej', 1, 0.5, temp_out_diagram_file)

    ##############
    # Humidity
    plot_set_ax_fig(today, t, h_out, values_count-1, 'g-', 'Wilgotnosc wzgledna [%]', 'Wykres wilgotnosci wzglednej', 5, 1, humid_out_diagram_file)

    ##############
    # Dew point
    plot_set_ax_fig(today, t, d_out, values_count-1, 'b-', 'Temp. punktu rosy [C]', 'Wykres temperatury punktu rosy', 1, 1, dew_point_out_diagram_file)

    ##############
    # Pressure
    plot_set_ax_fig(today, t, p_out, values_count-1, 'm-', 'Cisnienie atm. [hPa]', 'Wykres cisnienia atmosferycznego', 2, 1, pressure_diagram_file)

    return


# Draw a plot
def draw_plot_month_db():

    # Today
    today = datetime.datetime.today()
    plot_date_begin = datetime.datetime(today.year, today.month-1, 1)
    plot_date_end   = datetime.datetime(today.year, today.month, 1)

    t = []; # time axis for plot
    t_out = []; # temp out for plot
    h_out = []; # humid out for plot
    d_out = []; # dew point for plot
    p_out = []; # pressure for plot

    rows = get_val_month_db(today.month-1, today.year)  # month, and year
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
    plot_set_ax_fig(today, t, t_out, values_count-1, 'r-', 'Temperatura [C]', 'Wykres temperatury zewnetrznej', 1, 0.5, temp_out_diagram_file)
    
    ##############
    # Humidity
    plot_set_ax_fig(today, t, h_out, values_count-1, 'g-', 'Wilgotnosc wzgledna [%]', 'Wykres wilgotnosci wzglednej', 5, 1, humid_out_diagram_file)

    ##############
    # Dew point
    plot_set_ax_fig(today, t, d_out, values_count-1, 'b-', 'Temp. punktu rosy [C]', 'Wykres temperatury punktu rosy', 1, 1, dew_point_out_diagram_file)

    ##############
    # Pressure
    plot_set_ax_fig(today, t, p_out, values_count-1, 'm-', 'Cisnienie atm. [hPa]', 'Wykres cisnienia atmosferycznego', 2, 1, pressure_diagram_file)

    return



# Main program:
draw_plot_month_db();
#draw_plot_month();
