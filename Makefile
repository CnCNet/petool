include rust.mk

RUST_OUT_DIR = .
REV          = $(shell git rev-parse --short @{0})

$(eval $(call RUST_CRATE,LIB,src/petool.rs,))

petool: $(LIB_OUT)
clean:  $(LIB_CLEAN)

.PHONY: clean
