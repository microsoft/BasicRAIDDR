** RaiddrBasic_InitRTL ***
  Copyright (c) Microsoft Corporation.
  Licensed under the MIT License.

For initializing Basic RAIDDR in RTL
For reference only

raiddrinit.c
  Compile in Windows (Developer Console: cl raiddrinit.c) or Linux (gcc raiddrinit.c -o raiddrinit)
basicraiddr_rtlbase.v
  Base RTL code file for Basic RAIDDR. Useful for all configurations

After compilation, use 'raiddrinit -?' to see command-line options
Generate header with raiddrinit and manually append contents of basicraiddr_rtlbase.v to the output
