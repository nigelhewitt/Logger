# Logging
I wanted a tool to inspect and report on the ADIF files that I get from my FT-8 ham radio
software (WSJT-X). The file is automatically generated and a bit bare bones so I wanted to
add the DXCC country and the DXCC codes so I could sort on them.

To get the DXCC country data I download a file from the ARRL that is intended to be
man-readable and unscramble that. I don't actually think it is very man readable as I
can't deduce any certain rules that it doesn't break a few line further down but I think
I have a handle on what is intended - aside from the Russian entries that I substitue out
with another string that I believe covers the actual callsigns used. It decodes all the
countries in my current log other than four that I don't actually believe in.

The ListView source is based on an old Microsoft example with my tweaks.

This is 'take one' with display and column sorting. I wanted to archive the country code
source safely as it took a while to derive and test.

I think the next move will be to consult LoTW and mark the 'confirmed' ones and possibly
eQSL as these are the services I use.