CC = g++
CFLAGS = -Wall -std=c++17
LDFLAGS = -lncurses

SRC_DIR = src
BIN_DIR = bin
SYSTEM_INSTALL_DIR = /usr/local/bin
USER_INSTALL_DIR = $(HOME)/.local/bin

DIRMON_SRC = $(SRC_DIR)/dirmon/dirmon.cpp
FILEVIEW_SRC = $(SRC_DIR)/fileview/fileview.cpp
FILESEARCH_SRC = $(SRC_DIR)/filesearch/filesearch.cpp

DIRMON = $(BIN_DIR)/dirmon
FILEVIEW = $(BIN_DIR)/fileview
FILESEARCH = $(BIN_DIR)/filesearch

ALIAS_DR = $(BIN_DIR)/dr
ALIAS_FV = $(BIN_DIR)/fv
ALIAS_FS = $(BIN_DIR)/fs

.PHONY: all clean install install-user executable

all: $(BIN_DIR) $(DIRMON) $(FILEVIEW) $(FILESEARCH) aliases executable
	@echo "[✓] Build completed successfully"

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)
	@echo "[✓] Created bin directory"

$(DIRMON): $(DIRMON_SRC)
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "[✓] Built dirmon"

$(FILEVIEW): $(FILEVIEW_SRC)
	@$(CC) $(CFLAGS) -o $@ $<
	@echo "[✓] Built fileview"

$(FILESEARCH): $(FILESEARCH_SRC)
	@$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "[✓] Built filesearch"

aliases: $(DIRMON) $(FILEVIEW) $(FILESEARCH)
	@ln -sf dirmon $(ALIAS_DR)
	@ln -sf fileview $(ALIAS_FV)
	@ln -sf filesearch $(ALIAS_FS)
	@echo "[✓] Created aliases"

executable:
	@chmod +x finview
	@echo "[✓] Made finview executable"

clean:
	rm -rf $(BIN_DIR)
	@echo "[✓] Cleaned build files"

install: all
	@echo "[⚙] Installing to $(SYSTEM_INSTALL_DIR)..."
	@install -d $(SYSTEM_INSTALL_DIR)
	@install -m 755 $(DIRMON) $(FILEVIEW) $(FILESEARCH) $(SYSTEM_INSTALL_DIR)
	@ln -sf dirmon $(SYSTEM_INSTALL_DIR)/dr
	@ln -sf fileview $(SYSTEM_INSTALL_DIR)/fv
	@ln -sf filesearch $(SYSTEM_INSTALL_DIR)/fs
	@install -m 755 finview $(SYSTEM_INSTALL_DIR)
	@echo "[✓] System-wide installation complete!"

install-user: all
	@echo "[⚙] Installing to $(USER_INSTALL_DIR)..."
	@mkdir -p $(USER_INSTALL_DIR)
	@rm -rf $(USER_INSTALL_DIR)/bin
	@rm -f $(USER_INSTALL_DIR)/dr
	@rm -f $(USER_INSTALL_DIR)/fv
	@rm -f $(USER_INSTALL_DIR)/fs
	@install -m 755 $(BIN_DIR)/dirmon $(BIN_DIR)/fileview $(BIN_DIR)/filesearch $(USER_INSTALL_DIR)
	@cd $(USER_INSTALL_DIR) && ln -sf dirmon dr
	@cd $(USER_INSTALL_DIR) && ln -sf fileview fv
	@cd $(USER_INSTALL_DIR) && ln -sf filesearch fs
	@install -m 755 finview $(USER_INSTALL_DIR)
	@echo "[✓] User installation complete!"
	@if ! echo $$PATH | grep -q "$(USER_INSTALL_DIR)"; then \
		echo "\nNOTE: Add $(USER_INSTALL_DIR) to your PATH to use these commands from anywhere."; \
		echo "Add this line to your ~/.bashrc or ~/.zshrc:"; \
		echo "export PATH=\"\$$PATH:$(USER_INSTALL_DIR)\""; \
		echo "Then run: source ~/.bashrc (or ~/.zshrc)"; \
	fi
