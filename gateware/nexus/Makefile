all: no2bootloader-nexus.bit

RTL_SRCS = $(addprefix rtl/, \
	led_blinker.v \
	picorv32.v \
	soc_picorv32_bridge.v \
	soc_bram.v \
	sysmgr.v \
	wb_epbuf.v \
	wb_spi.v \
	top.v \
)
USB_SRCS = $(addprefix no2usb-rtl/, \
	usb.v \
	usb_crc.v \
	usb_ep_buf.v \
	usb_ep_status.v \
	usb_phy.v \
	usb_rx_ll.v \
	usb_rx_pkt.v \
	usb_trans.v \
	usb_tx_ll.v \
	usb_tx_pkt.v \
)

MISC_SRCS = $(addprefix ../cores/no2misc/rtl/, \
	fifo_sync_shift.v \
	fifo_sync_ram.v \
	uart_rx.v \
	uart_tx.v \
	uart_wb.v \
	xclk_strobe.v \
	xclk_wb.v \
	ram_sdp.v \
	glitch_filter.v \
)

no2bootloader-nexus.json: $(RTL_SRCS) $(USB_SRCS) $(MISC_SRCS) ../../firmware/no2bootloader-nexus.hex
	yosys -p "synth_nexus -top top -json no2bootloader-nexus.json" $(RTL_SRCS) $(USB_SRCS) $(MISC_SRCS)

no2bootloader-nexus.fasm: no2bootloader-nexus.json data/top-lifcl40-evn.pdc
	nextpnr-nexus --json no2bootloader-nexus.json --fasm no2bootloader-nexus.fasm --device LIFCL-40-9BG400CES --pdc data/top-lifcl40-evn.pdc

no2bootloader-nexus.bit: no2bootloader-nexus.fasm
	prjoxide pack no2bootloader-nexus.fasm no2bootloader-nexus.bit

prog: no2bootloader-nexus.bit
	ecpprog -S no2bootloader-nexus.bit

