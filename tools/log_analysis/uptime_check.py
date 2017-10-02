#!/usr/bin/env python
import os
import glob
from datetime import datetime
from dateutil.parser import parse as du_parse


def main():
    log_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "data", "log"))
    log_files = glob.glob(os.path.join(log_dir, "*.log"))

    boot_times = []

    for log_file in log_files:
        with open(log_file, "r") as f:
            for line in f:
                # Check for arbitrarily chosen bootup message
                if line.strip().endswith(":: Initializing channels."):
                    timestamp = line.split('::')[0].strip()
                    boot_times.append(du_parse(timestamp))

    boot_times.sort()

    max_result = None
    results = []
    for i in xrange(1, len(boot_times)):
        start = boot_times[i - 1]
        end = boot_times[i]
        diff = end - start
        
        result = (diff, start, end)

        if max_result is None or diff >= max_result[0]:
            max_result = result 
         

    curr_start = boot_times[-1]
    curr_end = datetime.now()
    curr_diff = curr_end - curr_start

    curr_result = (curr_diff, curr_start, curr_end)

    print "Analaysis of logs since: {}".format(boot_times[0])
    print "Longest uptime was {}, from {} to {}".format(*max_result)
    print "Current uptime is {}, from {} to {} (now)".format(*curr_result)
    
    # TODO: support export of all results to csv


if __name__ == "__main__":
    main()
