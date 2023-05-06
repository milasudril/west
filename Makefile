.PHONY: release

.PHONY: debug
.PHONY: clean
.PHONY: coverage
.PHONY: coverage-build

release:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-rel.json --target-dir=__targets_rel

debug:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-dbg.json --target-dir=__targets_dbg

clean:
	rm -rf __targets_*

coverage: __targets_gcov/.coverage/coverage.html

coverage-build:
	maike2 --configfiles=maikeconfig2.json,maikeconfig2-gcov.json --target-dir=__targets_gcov

__targets_gcov/.coverage/coverage.html: coverage-build ./coverage_collect.sh
	./coverage_collect.sh

