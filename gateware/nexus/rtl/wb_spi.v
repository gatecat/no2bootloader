`default_nettype none
module wb_spi (
	input wire clk,
	input wire rst,

	// Wishbone peripheral
	input  wire [3:0] wb_addr,
	output reg [31:0] wb_rdata,
	input  wire [31:0] wb_wdata,
	input  wire          wb_we,
	input  wire          wb_cyc,
	output wire          wb_ack,

	// SPI interface
	output wire spi_clk,
	output wire spi_csn,
	output wire spi_mosi,
	input wire spi_miso
);

	localparam REG_CONFIG = 0;
	localparam REG_DIVIDER = 1;
	localparam REG_TXDATA = 2;
	localparam REG_RXDATA = 3;
	localparam REG_STATUS = 4;

	reg [7:0] reg_config;
	reg [7:0] reg_divider;

	wire sck_idle = reg_config[0];
	wire sck_edge = reg_config[1];
	assign spi_csn = ~reg_config[2];

	reg [7:0] tx_data;
	reg [7:0] sr_o, sr_i;

	reg start_xfer;
	reg busy;

	wire [7:0] status;
	assign status[0] = busy;
	assign status[7:1] = 7'b0;

	reg ack;

	always @(posedge clk) begin
		if (rst) begin
			ack <= 1'b0;
			wb_rdata <= 1'b0;
			reg_config <= 8'b0;
			tx_data <= 8'b0;
			start_xfer <= 1'b0;
		end else begin
			ack <= wb_cyc & ~ack;

			wb_rdata <= 32'b0;
			start_xfer <= 1'b0;
			if (wb_cyc & ~ack) begin
				if (wb_addr == REG_CONFIG) begin
					if (wb_we) reg_config <= wb_wdata[7:0];
					wb_rdata <= reg_config;
				end else if (wb_addr == REG_DIVIDER) begin
					if (wb_we) reg_divider <= wb_wdata[7:0];
					wb_rdata <= reg_divider;
				end else if (wb_addr == REG_TXDATA) begin
					if (wb_we) begin
						tx_data <= wb_wdata;
						start_xfer <= 1'b1;
					end
				end else if (wb_addr == REG_RXDATA) begin
					wb_rdata <= sr_i;
				end else if (wb_addr == REG_STATUS) begin
					wb_rdata <= status;
				end
			end
		end
	end

	assign wb_ack = ack;

	localparam STATE_IDLE = 0;
	localparam STATE_ACTIVE = 1;
	localparam STATE_DONE = 2;
	reg sck;
	reg [1:0] state;
	reg [7:0] div_ctr;
	reg [3:0] bit_cnt;

	always @(posedge clk) begin
		if (rst) begin
			state <= STATE_IDLE;
			sck <= 1'b0;
			busy <= 1'b0;
			div_ctr <= 0;
			bit_cnt <= 0;
		end else begin
			if (state == STATE_IDLE) begin
				busy <= 1'b0;
				if (start_xfer) begin
					state = STATE_ACTIVE;
					busy <= 1'b1;
					div_ctr <= 0;
					bit_cnt <= 0;
				end
			end else if (state == STATE_ACTIVE) begin
				if (div_ctr == reg_divider) begin
					if (sck) begin
						if (bit_cnt == 7) begin
							state <= STATE_DONE;
						end else begin
							bit_cnt <= bit_cnt + 1;
						end
					end
					sck <= ~sck;
					div_ctr <= 0;
				end else begin
					div_ctr <= div_ctr + 1'b1;
				end
			end else if (state == STATE_DONE) begin
				state <= STATE_IDLE;
			end
		end
	end

	wire setup = (state == STATE_ACTIVE) && (div_ctr == reg_divider) && (sck == sck_edge);
	wire latch = (state == STATE_ACTIVE) && (div_ctr == reg_divider) && (sck == ~sck_edge);

	always @(posedge clk) begin
		if (start_xfer) begin
			sr_o <= tx_data;
			sr_i <= 0;
		end else begin
			if (setup) sr_o <= {sr_o[6:0], 1'b0};
			if (latch) sr_i <= {sr_i[6:0], spi_miso};
		end
	end

	assign spi_clk = sck ^ sck_idle;
	assign spi_mosi = sr_o[7];


endmodule
