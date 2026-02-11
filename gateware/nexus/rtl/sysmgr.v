/*
 * sysmgr.v
 *
 * vim: ts=4 sw=4
 *
 * Copyright (C) 2019-2020  Sylvain Munaut <tnt@246tNt.com>
 * SPDX-License-Identifier: CERN-OHL-P-2.0
 */

`default_nettype none
`include "boards.vh"

module sysmgr (
	input  wire clk_in,
	input  wire rst_in,
	output wire clk_24m,
	output wire clk_48m,
	output wire rst_out
);

	// Signals
	wire pll_lock;
	wire pll_reset_n;

	wire clk_24m_i;
	wire clk_48m_i;
	reg [3:0] rst_cnt;

	wire clk_fb;

	PLL #(
		.BW_CTL_BIAS("0b1111"),
		.CLKMUX_FB("CMUX_CLKOS5"),
		.CRIPPLE("3P"),
		.CSET("8P"),
		.DELA("20"),
		.DELB("40"),
		.DELF("80"),
		.DIVA("20"),
		.DIVB("40"),
		.DIVF("80"),
		.ENCLK_CLKOP("ENABLED"),
		.ENCLK_CLKOS("ENABLED"),
		.ENCLK_CLKOS5("ENABLED"),
		.FBK_INTEGER_MODE("ENABLED"),
		.FBK_MASK("0b00000000"),
		.FBK_MMD_DIG("1"),
		.IPI_CMP("0b1100"),
		.IPI_CMPN("0b0011"),
		.IPP_CTRL("0b0110"),
		.IPP_SEL("0b1111"),
		.KP_VCO("0b00011"),
		.PHIA("0"),
		.PLLPD_N("USED"),
		.PLLRESET_ENA("ENABLED"),
		.REF_INTEGER_MODE("ENABLED"),
		.REF_MMD_DIG("1"),
		.SEL_FBK("FBKCLK5"),
		.V2I_1V_EN("ENABLED"),
		.V2I_KVCO_SEL("60"),
		.V2I_PP_ICTRL("0b11111"),
		.V2I_PP_RES("10K"),

	) PLL_0 (
		.FBKCK(clk_fb),
		.PLLRESET(rst_in),
		.REFCK(clk_in),
		.CLKOP(clk_48m_i),
		.CLKOS(clk_24m_i),
		.CLKOS5(clk_fb),
		.LOCK(pll_lock)
	);

	assign clk_24m = clk_24m_i;
	assign clk_48m = clk_48m_i;

	// PLL reset generation

	// Logic reset generation
	always @(posedge clk_24m_i or negedge pll_lock)
		if (!pll_lock)
			rst_cnt <= 4'h0;
		else if (~rst_cnt[3])
			rst_cnt <= rst_cnt + 1;

	assign rst_out = ~rst_cnt[3];


endmodule // sysmgr
