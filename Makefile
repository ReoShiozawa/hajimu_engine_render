# ==============================================================================
# jp-engine_render — はじむ用 2D レンダリングエンジン
#
# 技術スタック: SDL2 + OpenGL 3.3 + stb_image + stb_truetype
#
# 使い方:
#   make vendor        stb ヘッダーをダウンロード（初回のみ）
#   make               ビルド
#   make install       ~/.hajimu/plugins/ にインストール
#   make clean         ビルド成果物を削除
#   make example       サンプルを実行
# ==============================================================================

PLUGIN_NAME  = engine_render
BUILD_DIR    = build
INSTALL_DIR  = $(HOME)/.hajimu/plugins/$(PLUGIN_NAME)
OUTPUT       = $(BUILD_DIR)/$(PLUGIN_NAME).hjp

VENDOR_DIR   = vendor
STB_IMAGE    = $(VENDOR_DIR)/stb_image.h
STB_TRUETYPE = $(VENDOR_DIR)/stb_truetype.h

# ── カラー出力 ──────────────────────────────────────────
RESET  = \033[0m
BOLD   = \033[1m
GREEN  = \033[32m
CYAN   = \033[36m
YELLOW = \033[33m

# ── ターゲット ──────────────────────────────────────────
.PHONY: all vendor clean install uninstall example help

all: vendor $(BUILD_DIR)/Makefile
	@echo "$(CYAN)▶ ビルド中...$(RESET)"
	@cmake --build $(BUILD_DIR) -j$(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
	@echo "$(GREEN)✅ ビルド完了: $(OUTPUT)$(RESET)"

$(BUILD_DIR)/Makefile: CMakeLists.txt
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -Wno-dev

vendor: $(STB_IMAGE) $(STB_TRUETYPE)

$(STB_IMAGE):
	@mkdir -p $(VENDOR_DIR)
	@echo "$(YELLOW)⬇ stb_image.h をダウンロード中...$(RESET)"
	@curl -fsSL -o $(STB_IMAGE) \
	    https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
	@echo "$(GREEN)✅ stb_image.h$(RESET)"

$(STB_TRUETYPE):
	@mkdir -p $(VENDOR_DIR)
	@echo "$(YELLOW)⬇ stb_truetype.h をダウンロード中...$(RESET)"
	@curl -fsSL -o $(STB_TRUETYPE) \
	    https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
	@echo "$(GREEN)✅ stb_truetype.h$(RESET)"

install: all
	@mkdir -p $(INSTALL_DIR)
	@cp $(OUTPUT) $(INSTALL_DIR)/
	@echo "$(GREEN)✅ インストール完了: $(INSTALL_DIR)$(RESET)"

uninstall:
	@rm -rf $(INSTALL_DIR)
	@echo "$(GREEN)✅ 削除完了$(RESET)"

example: install
	@cd examples && hajimu hello_sprite.jp

clean:
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✅ クリーン完了$(RESET)"

help:
	@echo "$(BOLD)jp-engine_render$(RESET) — はじむ用 2D レンダリングエンジン"
	@echo ""
	@echo "  $(CYAN)make vendor$(RESET)    stbヘッダーをダウンロード（初回のみ）"
	@echo "  $(CYAN)make$(RESET)           ビルド"
	@echo "  $(CYAN)make install$(RESET)   ~/.hajimu/plugins/ にインストール"
	@echo "  $(CYAN)make clean$(RESET)     ビルド成果物を削除"
