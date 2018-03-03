WORK_DIR = $(HOME)/work/afl/afl-pt/call-trace-analysis/

BIN_PNG = readpng
TEST_DIR_PNG_Q = libpng/libpng-qemu-long-12-3/
TRACE_FILE_PNG_Q = png-qemu.calltrace
readpng-qemu:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_PNG)" -b $(WORK_DIR)/test_targets/$(BIN_PNG) -d test_data/$(TEST_DIR_PNG_Q) -o $(TRACE_FILE_PNG_Q)

readpng-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_PNG_Q) > $(TRACE_FILE_PNG_Q).analysis

TEST_DIR_PNG_PT = libpng/libpng-ptfast-long-12-3/
TRACE_FILE_PNG_PT = png-ptfast.calltrace
readpng-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_PNG)" -b $(WORK_DIR)/test_targets/$(BIN_PNG) -d test_data/libpng/$(TEST_DIR_PNG_PT) -o $(TRACE_FILE_PNG_PT) 

readpng-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_PNG_PT) > $(TRACE_FILE_PNG_PT).analysis

BIN_CXXFILT = cxxfilt
TEST_DIR_CXX_Q = c++filt/cxxfilt-qemu-long-12-3/
TRACE_FILE_CXX_Q = cxxfilt-qemu.calltrace
cxx-qemu:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_CXXFILT)" -b $(WORK_DIR)/test_targets/$(BIN_CXXFILT) -d test_data/$(TEST_DIR_CXX_Q) -o $(TRACE_FILE_CXX_Q)

cxx-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_CXX_Q) > $(TRACE_FILE_CXX_Q).analysis

TEST_DIR_CXX_PT = c++filt/cxxfilt-ptfast-long-12-3/
TRACE_FILE_CXX_PT = cxxfilt-ptfast.calltrace
cxx-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_CXXFILT)" -b $(WORK_DIR)/test_targets/$(BIN_CXXFILT) -d test_data/$(TEST_DIR_CXX_PT) -o $(TRACE_FILE_CXX_PT)

cxx-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_CXX_PT) > $(TRACE_FILE_CXX_PT).analysis

BIN_EXIF = exif
TEST_DIR_EXIF_Q = exif/exif-qemu-long-12-3/
TRACE_FILE_EXIF_Q = exif-qemu.calltrace
exif-qemu:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_EXIF)" -b $(WORK_DIR)/test_targets/$(BIN_EXIF) -d test_data/$(TEST_DIR_EXIF_Q) -o $(TRACE_FILE_EXIF_Q)

exif-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_EXIF_Q) > $(TRACE_FILE_EXIF_Q).analysis

TEST_DIR_EXIF_PT = exif/exif-ptfast-long-2-12/
TRACE_FILE_EXIF_PT = exif-ptfast.calltrace
exif-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_EXIF)" -b $(WORK_DIR)/test_targets/$(BIN_EXIF) -d test_data/$(TEST_DIR_EXIF_PT) -o $(TRACE_FILE_EXIF_PT)

exif-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_EXIF_PT) > $(TRACE_FILE_EXIF_PT).analysis

BIN_JPEG = djpeg
TEST_DIR_JPEG_Q = libjpeg/jpeg-qemu-long-12-3/
TRACE_FILE_JPEG_Q = djpeg-qemu.calltrace
djpeg-qemu:
	python ./batch_run_wrapper.p -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_JPEG)" -b $(WORK_DIR)/test_targets/$(BIN_JPEG) -d test_data/$(TEST_DIR_JPEG_Q) -o $(TRACE_FILE_JPEG_Q)

djpeg-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_JPEG_Q) > $(TRACE_FILE_JPEG_Q).analysis

TEST_DIR_JPEG_PT = libjpeg/jpeg-ptfast-long-12-16/
TRACE_FILE_JPEG_PT = djpeg-ptfast.calltrace
djpeg-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_JPEG)" -b $(WORK_DIR)/test_targets/libjpeg/ -d test_data/$(TEST_DIR_JPEG_PT) -o $(TRACE_FILE_JPEG_PT)

djpeg-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_JPEG_PT) > $(TRACE_FILE_JPEG_PT).analysis

BIN_NM = nm
TEST_DIR_NM_Q = nm/nm-qemu-long-12-3/
TRACE_FILE_NM_Q = nm-qemu.calltrace
nm-qemu:
	python ./batch_run_wrapper.p -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_NM)" -b $(WORK_DIR)/test_targets/$(BIN_NM) -d test_data/$(TEST_DIR_NM_Q) -o $(TRACE_FILE_NM_Q)

nm-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_NM_Q) > $(TRACE_FILE_NM_Q).analysis

TEST_DIR_NM_PT = nm/nm-ptfast-long-12-3/
TRACE_FILE_NM_PT = nm-ptfast.calltrace
nm-ptfast:
	python ./batch_run_wrapper.p -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_NM)" -b $(WORK_DIR)/test_targets/$(BIN_NM) -d test_data/$(TEST_DIR_NM_PT) -o $(TRACE_FILE_NM_PT)

nm-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_NM_PT) > $(TRACE_FILE_NM_PT).analysis
