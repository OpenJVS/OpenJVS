.PHONY: default install clean

BUILD = build

default: $(BUILD)/Makefile
	@cd $(BUILD) && $(MAKE)

install: default
	@cd $(BUILD) && cpack
	@sudo dpkg --install $(BUILD)/*.deb
clean:
	@rm -rf $(BUILD)

$(BUILD)/Makefile:
	@mkdir -p $(BUILD)
	@cd $(BUILD) && cmake ..
