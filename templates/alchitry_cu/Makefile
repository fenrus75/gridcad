all: synth/main.bin

synth/main.json: main.v
	mkdir -p synth/
	yosys -p 'synth_ice40 -top main -json synth/main.json' main.v
	
synth/main.asc: synth/main.json
	nextpnr-ice40 --hx8k --package cb132 --json synth/main.json --pcf cu.pcf --asc synth/main.asc
	
synth/main.bin: synth/main.asc
	icepack synth/main.asc synth/main.bin
	


program: synth/main.bin
	iceprog synth/main.bin
