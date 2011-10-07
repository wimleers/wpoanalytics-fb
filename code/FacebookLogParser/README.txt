How to generate a data dump that can be read by this parser.

Supported data sets:
- perfpipe_cavalry
- perfpipe_doppler_cdn

1) ssh onto your sandbox
2) run the following command: `ptail -f <data set name> -dpc true -time 2011-11-06T12:00 > perfpipe_cavalry.json` (change parameters as you like)
3) scp it to your local machine
4) run the prototype
