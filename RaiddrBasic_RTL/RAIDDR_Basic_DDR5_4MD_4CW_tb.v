// *** RAIDDR_Basic_DDR5_4MD_4CW_tb.v ***
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Example Basic RAIDDR RTL Testbench file for RAIDDR_Basic_DDR5_4MD_4CW.v
// Targets serveral known ECC gap patterns for this configuration and also failure patterns which should be correctable
// Questions? Contact: Brett Dodds (brett.dodds@microsoft.com)

module raiddr_tb_DDR5_10x4;
  reg [511:0] wdata;
  reg [3:0] wmd;
  reg [639:0] codeword_in;
  reg [511:0] rdata;
  reg [3:0] rmd;
  reg [639:0] codeword_out;
  reg ue;
  reg ce;
  reg [63:0] ceMask;
  reg [9:0] symMask;
  reg [63:0] errpattern[8];
  integer sym, e, b;
  
  raiddr_enc encodeinst(
    .data(wdata),
    .metadata(wmd),
    .codeword(codeword_in)
  );
  
  raiddr_dec decodeinst(
    .codeword(codeword_out),
    .data(rdata),
    .metadata(rmd),
    .ue(ue),
    .ce(ce),
    .ceMask(ceMask),
    .symMask(symMask)
  );
  
  initial begin
    errpattern = { 64'hdeadbeefdeadbeef, 64'h2af32af32af32af3, 64'h55e655e655e655e6, 64'h903c903c903c903c, 64'he028e028e028e028, 64'h813f813f813f813f, 64'hfe2afe2afe2afe2a, 64'h7014701470147014 };
    wdata = 512'h0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF;
    wmd = 4'b1101;
    #1
    codeword_out = codeword_in;
    #1
    $display("%h+%h (%h) -> %h+%h -> ue=%d ce=%d ceMask=%h symMask=%h", wmd, wdata, codeword_out, rmd, rdata, ue, ce, ceMask, symMask);
    for (e = 0; e < 8; e = e + 1) begin
      for (sym = 0; sym < 640; sym = sym + 64) begin
            codeword_out = codeword_in;
            for (b = sym; b < sym + 64; b = b + 1) codeword_out[b] = codeword_in[b] ^ errpattern[e][b - sym];
            #1
            $display("%h+%h (%h) -> %h+%h -> ue=%d ce=%d ceMask=%h symMask=%h", wmd, wdata, codeword_out, rmd, rdata, ue, ce, ceMask, symMask);
        end
    end 
    //$dumpfile("dump.vcd"); $dumpvars;
  end
endmodule
