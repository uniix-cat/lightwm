.PHONY: srpm

outdir ?= .
SPEC := packaging/lightwm.spec
VERSION := $(shell awk '/^Version:/ {print $$2}' $(SPEC))
NAME := lightwm

srpm:
	@mkdir -p "$(outdir)"
	@topdir=$$(mktemp -d); \
	trap 'rm -rf "$$topdir"' EXIT; \
	mkdir -p "$$topdir/SOURCES" "$$topdir/SPECS" "$$topdir/SRPMS"; \
	if [ -d .git ]; then \
		git archive --format=tar.gz --prefix=$(NAME)-$(VERSION)/ -o "$$topdir/SOURCES/$(NAME)-$(VERSION).tar.gz" HEAD; \
	else \
		tar --transform "s,^\.,$(NAME)-$(VERSION)," --exclude=.git --exclude=build -czf "$$topdir/SOURCES/$(NAME)-$(VERSION).tar.gz" .; \
	fi; \
	cp "$(SPEC)" "$$topdir/SPECS/$(NAME).spec"; \
	rpmbuild -bs --define "_topdir $$topdir" --define "_srcrpmdir $(abspath $(outdir))" "$$topdir/SPECS/$(NAME).spec"
