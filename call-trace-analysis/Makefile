WORK_DIR = $(HOME)/work/afl-pt/call-trace-analysis/

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
TEST_DIR_EXIF_Q1 = exif/exif-qemu-long-12-3/
TRACE_FILE_EXIF_Q1 = exif-qemu1.calltrace
TEST_DIR_EXIF_Q2 = exif/exif-qemu-long-2-7/
TRACE_FILE_EXIF_Q2 = exif-qemu2.calltrace
exif-qemu:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_EXIF) AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_EXIF) -d test_data/$(TEST_DIR_EXIF_Q1) -o $(TRACE_FILE_EXIF_Q1)

	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_EXIF) AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_EXIF) -d test_data/$(TEST_DIR_EXIF_Q2) -o $(TRACE_FILE_EXIF_Q2)
exif-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_EXIF_Q1) > $(TRACE_FILE_EXIF_Q1).analysis
	python ./CallTraceParser.py $(TRACE_FILE_EXIF_Q2) > $(TRACE_FILE_EXIF_Q2).analysis


TEST_DIR_EXIF_PT = exif/exif-ptfast-long-2-12/
TRACE_FILE_EXIF_PT = exif-ptfast.calltrace
exif-ptfast:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_EXIF) AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_EXIF) -d test_data/$(TEST_DIR_EXIF_PT) -o $(TRACE_FILE_EXIF_PT)

exif-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_EXIF_PT) > $(TRACE_FILE_EXIF_PT).analysis

BIN_JPEG = djpeg
TEST_DIR_JPEG_Q = libjpeg/jpeg-qemu-long-12-3/
TRACE_FILE_JPEG_Q = djpeg-qemu.calltrace
djpeg-qemu:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_JPEG) -gif" -b $(WORK_DIR)/test_targets/$(BIN_JPEG) -d test_data/$(TEST_DIR_JPEG_Q) -o $(TRACE_FILE_JPEG_Q)

djpeg-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_JPEG_Q) > $(TRACE_FILE_JPEG_Q).analysis

TEST_DIR_JPEG_PT1 = libjpeg/jpeg-ptfast-long-12-16/
TRACE_FILE_JPEG_PT1 = djpeg-ptfast1.calltrace
TEST_DIR_JPEG_PT2 = libjpeg/jpeg-ptfast-long-12-3/
TRACE_FILE_JPEG_PT2 = djpeg-ptfast2.calltrace
djpeg-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_JPEG) -gif" -b $(WORK_DIR)/test_targets/libjpeg/ -d test_data/$(TEST_DIR_JPEG_PT1) -o $(TRACE_FILE_JPEG_PT1)
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_JPEG)" -b $(WORK_DIR)/test_targets/libjpeg/ -d test_data/$(TEST_DIR_JPEG_PT2) -o $(TRACE_FILE_JPEG_PT2)

djpeg-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_JPEG_PT1) > $(TRACE_FILE_JPEG_PT1).analysis
	python ./CallTraceParser.py $(TRACE_FILE_JPEG_PT2) > $(TRACE_FILE_JPEG_PT2).analysis

BIN_NM = nm-new
TEST_DIR_NM_Q1 = nm/nm-qemu-long-12-3/
TRACE_FILE_NM_Q1 = nm-qemu1.calltrace
TEST_DIR_NM_Q2 = nm/nm-qemu-long-2-7/
TRACE_FILE_NM_Q2 = nm-qemu2.calltrace
nm-qemu:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_NM)" -b $(WORK_DIR)/test_targets/$(BIN_NM) -d test_data/$(TEST_DIR_NM_Q1) -o $(TRACE_FILE_NM_Q1)
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_NM)" -b $(WORK_DIR)/test_targets/$(BIN_NM) -d test_data/$(TEST_DIR_NM_Q2) -o $(TRACE_FILE_NM_Q2)

nm-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_NM_Q1) > $(TRACE_FILE_NM_Q1).analysis
	python ./CallTraceParser.py $(TRACE_FILE_NM_Q2) > $(TRACE_FILE_NM_Q2).analysis

TEST_DIR_NM_PT = nm/nm-ptfast-long-12-3/
TRACE_FILE_NM_PT = nm-ptfast.calltrace
nm-ptfast:
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_NM)" -b $(WORK_DIR)/test_targets/$(BIN_NM) -d test_data/$(TEST_DIR_NM_PT) -o $(TRACE_FILE_NM_PT)

nm-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_NM_PT) > $(TRACE_FILE_NM_PT).analysis


BIN_MUTOOL = mutool
TEST_DIR_MUTOOL_Q = mupdf/mupdf-qemu-long-12-3/
TRACE_FILE_MUTOOL_Q = mupdf-qemu.calltrace
mutool-qemu:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_MUTOOL) show AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_MUTOOL) -d test_data/$(TEST_DIR_MUTOOL_Q) -o $(TRACE_FILE_MUTOOL_Q)

mutool-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_MUTOOL_Q) > $(TRACE_FILE_MUTOOL_Q).analysis

TEST_DIR_MUTOOL_PT = mupdf/mutool-ptfast-long-12-3/
TRACE_FILE_MUTOOL_PT = mupdf-ptfast.calltrace
mutool-ptfast:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_MUTOOL) show AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_MUTOOL) -d test_data/$(TEST_DIR_MUTOOL_PT) -o $(TRACE_FILE_MUTOOL_PT)

mutool-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_MUTOOL_PT) > $(TRACE_FILE_MUTOOL_PT).analysis

BIN_OBJDUMP = objdump
TEST_DIR_OBJDUMP_Q = objdump/objdump-qemu-long-12-3/
TRACE_FILE_OBJDUMP_Q = objdump-qemu.calltrace
objdump-qemu:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -D AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -d test_data/$(TEST_DIR_OBJDUMP_Q) -o $(TRACE_FILE_OBJDUMP_Q)

objdump-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_OBJDUMP_Q) > $(TRACE_FILE_OBJDUMP_Q).analysis

TEST_DIR_OBJDUMP_PT1 = objdump/objdump-ptfast-long-12-3/
TRACE_FILE_OBJDUMP_PT1 = objdump-ptfast1.calltrace
TEST_DIR_OBJDUMP_PT2 = objdump/objdump-ptfast-long-12-16/
TRACE_FILE_OBJDUMP_PT2 = objdump-ptfast2.calltrace
objdump-ptfast:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -D AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -d test_data/$(TEST_DIR_OBJDUMP_PT1) -o $(TRACE_FILE_OBJDUMP_PT1)

	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -D AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_OBJDUMP) -d test_data/$(TEST_DIR_OBJDUMP_PT2) -o $(TRACE_FILE_OBJDUMP_PT2)
objdump-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_OBJDUMP_PT1) > $(TRACE_FILE_OBJDUMP_PT1).analysis
	python ./CallTraceParser.py $(TRACE_FILE_OBJDUMP_PT2) > $(TRACE_FILE_OBJDUMP_PT2).analysis

BIN_XMLLINT = xmllint
TEST_DIR_XML_Q = libxml/xmllint-qemu-long-12-3/
TRACE_FILE_XML_Q = xmllint-qemu.calltrace
xmllint-qemu:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_XMLLINT) AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_XMLLINT) -d test_data/$(TEST_DIR_XML_Q) -o $(TRACE_FILE_XML_Q)

xmllint-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_XML_Q) > $(TRACE_FILE_XML_Q).analysis

TEST_DIR_XML_PT = libxml/xmllint-ptfast-long-12-3/
TRACE_FILE_XML_PT = xmllint-ptfast.calltrace
xmllint-ptfast:
	python ./batch_run_wrapper.py -e "$(WORK_DIR)/test_targets/$(BIN_XMLLINT) AFL_FILE" -b $(WORK_DIR)/test_targets/$(BIN_XMLLINT) -d test_data/$(TEST_DIR_XML_PT) -o $(TRACE_FILE_XML_PT)

xmllint-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_XML_PT) > $(TRACE_FILE_XML_PT).analysis


BIN_PERL = perl
TEST_DIR_PERL_Q = perl/perl-qemu-long-12-3/
TRACE_FILE_PERL_Q = perl-qemu.calltrace
perl-qemu:
	rm -rf perl_sandbox; mkdir perl_sandbox; cd perl_sandbox
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_PERL)" -b $(WORK_DIR)/test_targets/$(BIN_PERL) -d test_data/$(TEST_DIR_PERL_Q) -o ../$(TRACE_FILE_PERL_Q)
	cd $(WORK_DIR)

perl-qemu-parse:
	python ./CallTraceParser.py $(TRACE_FILE_PERL_Q) > $(TRACE_FILE_PERL_Q).analysis


TEST_DIR_PERL_PT1 = perl/perl-ptfast-long-12-3/
TRACE_FILE_PERL_PT1 = perl-ptfast1.calltrace
TEST_DIR_PERL_PT2 = perl/perl-ptfast-long-12-16/
TRACE_FILE_PERL_PT2 = perl-ptfast2.calltrace
perl-ptfast:
	rm -rf perl_sandbox; mkdir perl_sandbox; cd perl_sandbox
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_PERL)" -b $(WORK_DIR)/test_targets/$(BIN_PERL) -d test_data/$(TEST_DIR_PERL_PT1) -o $(TRACE_FILE_PERL_PT1)
	cd $(WORK_DIR)

	rm -rf perl_sandbox; mkdir perl_sandbox; cd perl_sandbox
	python ./batch_run_wrapper.py -e "cat AFL_FILE | $(WORK_DIR)/test_targets/$(BIN_PERL)" -b $(WORK_DIR)/test_targets/$(BIN_PERL) -d test_data/$(TEST_DIR_PERL_PT2) -o $(TRACE_FILE_PERL_PT2)
	cd $(WORK_DIR)
perl-ptfast-parse:
	python ./CallTraceParser.py $(TRACE_FILE_PERL_PT1) > $(TRACE_FILE_PERL_PT1).analysis
	python ./CallTraceParser.py $(TRACE_FILE_PERL_PT2) > $(TRACE_FILE_PERL_PT2).analysis
