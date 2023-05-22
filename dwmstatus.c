/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tz_india = "Asia/Kolkata";
char *tzutc = "UTC";
char *separator = "    ";
char *battery_base = "/sys/class/power_supply/BAT0";
char *battery_icons[] =          { "󰂎", "󰁺", "󰁻", "󰁼", "󰁽", "󰁾", "󰁿", "󰂀", "󰂁", "󰂂", "󰁹" };
char *battery_charging_icons[] = { "󰢟", "󰢜", "󰂆", "󰂇", "󰂈", "󰢝", "󰂉", "󰢞", "󰂊", "󰂋", "󰂅" };

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL) {
		fclose(fd);
		return NULL;
	}
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery()
{
	char *co, *status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(battery_base, "present");
	if (co == NULL)
		return smprintf("");
	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(battery_base, "charge_full_design");
	if (co == NULL) {
		co = readfile(battery_base, "energy_full_design");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(battery_base, "charge_now");
	if (co == NULL) {
		co = readfile(battery_base, "energy_now");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &remcap);
	free(co);

	char **battery_icon_list = battery_icons;

	co = readfile(battery_base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status = "";
	} else if(!strncmp(co, "Charging", 8)) {
		status = "";
		battery_icon_list = battery_charging_icons;
	} else {
		status = "";
	}

	if (remcap < 0 || descap < 0)
		return smprintf("invalid");

	float battery_percentage = ((float) remcap / (float) descap) * 10;
	char *battery_icon = battery_icon_list[(unsigned int) battery_percentage];

	return smprintf("%s%s %.0f%%", battery_icon, status, battery_percentage * 10);
}

char *
execscript(char *cmd)
{
	FILE *fp;
	char retval[1025], *rv;

	memset(retval, 0, sizeof(retval));

	fp = popen(cmd, "r");
	if (fp == NULL)
		return smprintf("");

	rv = fgets(retval, sizeof(retval), fp);
	pclose(fp);
	if (rv == NULL)
		return smprintf("");
	retval[strlen(retval)-1] = '\0';

	return smprintf("%s", retval);
}

int
main(void)
{
	char *status;
	char *bat;
	char *tm_india;
	char *da_india;
	char *kbmap;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return EXIT_FAILURE;
	}

	for (;;sleep(30)) {
		bat = getbattery();
		tm_india = mktimes("%H:%M", tz_india);
		da_india = mktimes("%a %d %b %Y", tz_india);
		kbmap = execscript("setxkbmap -query | grep layout | cut -d':' -f 2- | tr -d ' '");

		status = smprintf(
			" %s%s%s%s %s%s %s",
			kbmap, separator,
			bat, separator,
			tm_india, separator,
			da_india
		);
		setstatus(status);

		free(bat);
		free(tm_india);
		free(da_india);
		free(status);
	}

	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

