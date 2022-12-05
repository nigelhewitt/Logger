# Logging
I wanted a tool to inspect and report on the ADIF files that I get from my FT-8 ham radio
software (WSJT-X). The file is automatically generated and a bit bare bones so I wanted to
add the DXCC country and the DXCC codes so I could sort on them.

To get the DXCC country data I download a file from the ARRL that is intended to be
man-readable and unscramble that. I don't actually think it is very man readable as I
can't deduce any certain rules that it doesn't break but I think I have a handle on what
is intended - aside from the Russian entries that I substitue out with another string
that I believe covers the actual callsigns used. It decodes all the countries in my
current log other than four that I don't actually believe in.

The ListView source is based on an old Microsoft example with my tweaks.
The Unit tests are only for the bits that were better tested offline.
The filename is hardcoded but simple to fix. I will probably put it in an INI file later
and add a 'Select file' option.

This is 'take one' with ADI read, DXCC addition, display and column sorting. I wanted to
archive the country code source safely as it took a while to derive and test. All the log
analysis stuff will come later and probably not be useful to anybody else.

I consult my LoTW log and mark the 'confirmed' ones and possibly I will extend it to
eQSL as these are the services I use.

Please don't tell me that you wouldn't have done it this way as me laughing at you may
cause offence. Alternativly cut and stick what you want. I hoped to find an ADI file
reader but couldn't find one I liked online.

The configuration file is called config.edc and is a simple ascci 'ini' file eg:
```
[setup]
LOTWuser=a user name
LOTWpassword=a password
```

the system will add other stuff as needed.
