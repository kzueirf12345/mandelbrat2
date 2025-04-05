.PHONY: all build clean rebuild \
		logger_build logger_clean logger_rebuild \
		clean_all clean_log clean_out clean_obj clean_deps clean_txt clean_bin \
		analyze generate_analyze


PROJECT_NAME = mandelbrat2

BUILD_DIR = ./build
SRC_DIR = ./src
COMPILER ?= gcc

DEBUG_ ?= 1
USE_AVX2 ?= 1

ifeq ($(origin FLAGS), undefined)


ifeq ($(COMPILER),gcc)

FLAGS =	-Wall -Wextra -Waggressive-loop-optimizations \
		-Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts \
		-Wconversion -Wempty-body -Wfloat-equal \
		-Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
		-Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self \
		-Wredundant-decls -Wshadow -Wsign-conversion \
		-Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods \
		-Wsuggest-final-types -Wswitch-default -Wswitch-enum -Wsync-nand \
		-Wundef -Wunreachable-code -Wunused -Wvariadic-macros \
		-Wno-missing-field-initializers -Wno-narrowing -Wno-varargs \
		-Wstack-protector -fcheck-new -fstack-protector -fstrict-overflow \
		-fno-omit-frame-pointer -Wlarger-than=81920 -Wstack-usage=81920 -pie \
		-fPIE -Werror=vla -flto-odr-type-merging\

SANITIZER = -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,$\
		integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,$\
		shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

else

FLAGS = -Wall -Wextra \
        -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts \
        -Wconversion -Wempty-body -Wfloat-equal \
        -Wformat-security -Wformat-signedness -Wformat=2 -Winline \
        -Wpointer-arith -Winit-self \
        -Wredundant-decls -Wshadow \
        -Wstrict-overflow=2 -Wswitch-default -Wswitch-enum \
        -Wundef -Wunreachable-code -Wunused -Wvariadic-macros \
        -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs \
        -fstack-protector -fstrict-overflow \
        -fno-omit-frame-pointer \
        -fPIE -Werror=vla -flto=thin

SANITIZER = -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,\
            integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,shift,\
            signed-integer-overflow,undefined,unreachable,vptr

endif

DEBUG_FLAGS = -D _DEBUG  -ggdb -g3 -D_FORTIFY_SOURCES=3 $(SANITIZER)
RELEASE_FLAGS = -DNDEBUG

ifneq ($(DEBUG_),0)
OPTIMIZE_LVL ?= -Og
FLAGS += $(DEBUG_FLAGS) 
else
OPTIMIZE_LVL ?= -O2
FLAGS += $(RELEASE_FLAGS)
endif

FLAGS += $(OPTIMIZE_LVL)

ifneq ($(USE_AVX2),0)
FLAGS += -mavx -mavx2 -march=native -fopenmp
endif

endif

FLAGS += $(ADD_FLAGS)

LIBS = -lm -lSDL2 -lSDL2main -lSDL2_ttf -L./libs/logger -llogger


DIRS = utils flags mandelbrat2 time_checker sdl_objs
BUILD_DIRS = $(DIRS:%=$(BUILD_DIR)/%)

SOURCES = main.c utils/utils.c flags/flags.c mandelbrat2/mandelbrat2.c time_checker/time_checker.c	\
		  sdl_objs/sdl_objs.c

SOURCES_REL_PATH = $(SOURCES:%=$(SRC_DIR)/%)
OBJECTS_REL_PATH = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPS_REL_PATH = $(OBJECTS_REL_PATH:%.o=%.d)


all: build start

start:
	LD_LIBRARY_PATH=/usr/local/lib ./$(PROJECT_NAME).out $(OPTS)

build: $(PROJECT_NAME).out

rebuild: clean_all build



$(PROJECT_NAME).out: $(OBJECTS_REL_PATH)
	@$(COMPILER) $(FLAGS) -o $@ $^  $(LIBS)

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c | ./$(BUILD_DIR)/ $(BUILD_DIRS) logger_build
	@$(COMPILER) $(FLAGS) -I$(SRC_DIR) -I./libs -I./assets -c -MMD -MP $< -o $@

-include $(DEPS_REL_PATH)

$(BUILD_DIRS):
	mkdir $@
./$(BUILD_DIR)/:
	mkdir $@


logger_rebuild: logger_build logger_clean

logger_build:
	@make ADD_FLAGS="$(ADD_FLAGS)" FLAGS="$(FLAGS)" DEBUG_=$(DEBUG_) build -C ./libs/logger

logger_clean:
	make ADD_FLAGS="$(ADD_FLAGS)" clean -C ./libs/logger



clean_all: clean_obj clean_deps clean_out logger_clean

clean: clean_obj clean_deps clean_out

clean_log:
	rm -rf ./log/*

clean_out:
	rm -rf ./*.out

clean_obj:
	rm -rf ./$(OBJECTS_REL_PATH)

clean_deps:
	rm -rf ./$(DEPS_REL_PATH)

clean_txt:
	rm -rf ./*.txt

clean_bin:
	rm -rf ./*.bin

clean_gcda:
	sudo find ./ -type f -name "*.gcda" -exec rm -f {} \;

OUTPUTS = ./assets/gcc_none_O0 ./assets/gcc_O2  ./assets/gcc_O3  ./assets/gcc_avx2_O2 \
		  ./assets/gcc_avx2_O3  ./assets/gcc_avx2_unroll_O2 ./assets/gcc_avx2_unroll_O3  \
		  ./assets/gcc_array_unroll_O2 ./assets/gcc_array_unroll_O3 \
		  ./assets/clang_none_O0 ./assets/clang_O2 ./assets/clang_O3 \
		  ./assets/clang_avx2_O2 ./assets/clang_avx2_O3 ./assets/clang_avx2_unroll_O2 \
		  ./assets/clang_avx2_unroll_O3 ./assets/clang_array_unroll_O2 ./assets/clang_array_unroll_O3

GRAPHIC	= ./assets/histogram
STAT_CHECK = ./assets/stat_check

ANALYZE_NUM ?= 1

OUTPUTS_NUM = $(foreach out,$(OUTPUTS),$(out)$(ANALYZE_NUM).txt)
GRAPHIC_NUM = $(foreach graphic,$(GRAPHIC),$(graphic)$(ANALYZE_NUM).png)
STAT_CHECK_NUM = $(foreach stat_check,$(STAT_CHECK),$(stat_check)$(ANALYZE_NUM).png)

REP_CNT ?= 5
MEASURE_CNT ?= 20

generate_analyze: 
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O0 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'" 																										   OPTS="-r $(REP_CNT) -o ./assets/clang_none_O0$(ANALYZE_NUM).txt  	  	-c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                     -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate"					   OPTS="-r $(REP_CNT) -o ./assets/clang_O3$(ANALYZE_NUM).txt       	  	-c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                     -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_O3$(ANALYZE_NUM).txt       		-c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86          -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_O3$(ANALYZE_NUM).txt  		-c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86          -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_O3$(ANALYZE_NUM).txt  		-c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_unroll_O3$(ANALYZE_NUM).txt   -c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_unroll_O3$(ANALYZE_NUM).txt   -c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'   		   	    -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_array_unroll_O3$(ANALYZE_NUM).txt  -c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  			    -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_array_unroll_O3$(ANALYZE_NUM).txt  -c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	\
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O0 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'" 																				  OPTS="-r $(REP_CNT) -o ./assets/gcc_none_O0$(ANALYZE_NUM).txt  	  	   -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                      -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_O3$(ANALYZE_NUM).txt       	  	   -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                      -ffast-math -funroll-loops -flto -mfma -fprofile-use"      OPTS="-r $(REP_CNT) -o ./assets/gcc_O3$(ANALYZE_NUM).txt       		   -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86           -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_O3$(ANALYZE_NUM).txt  		   -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86           -ffast-math -funroll-loops -flto -mfma -fprofile-use"   	  OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_O3$(ANALYZE_NUM).txt  		   -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL  -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_unroll_O3$(ANALYZE_NUM).txt     -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL  -ffast-math -funroll-loops -flto -mfma -fprofile-use" 	  OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_unroll_O3$(ANALYZE_NUM).txt     -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  			       -ffast-math -funroll-loops -flto -mfma" 		              OPTS="-r $(REP_CNT) -o ./assets/gcc_array_unroll_O3$(ANALYZE_NUM).txt    -c $(MEASURE_CNT)" rebuild all ;
	\
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                     -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate"					   OPTS="-r $(REP_CNT) -o ./assets/clang_O2$(ANALYZE_NUM).txt       	  	-c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                     -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_O2$(ANALYZE_NUM).txt       		-c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86          -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_O2$(ANALYZE_NUM).txt  		-c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86          -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_O2$(ANALYZE_NUM).txt  		-c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_unroll_O2$(ANALYZE_NUM).txt   -c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_avx2_unroll_O2$(ANALYZE_NUM).txt   -c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'   		   	    -ffast-math -funroll-loops -flto -mfma -fprofile-instr-generate" 					   OPTS="-r $(REP_CNT) -o ./assets/clang_array_unroll_O2$(ANALYZE_NUM).txt  -c $(MEASURE_CNT)" rebuild all ;
	sudo llvm-profdata merge -output=./program.profdata ./default.profraw ;
	sudo nice -n -20 make COMPILER=clang DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  			    -ffast-math -funroll-loops -flto -mfma -fprofile-instr-use=$(CURDIR)/program.profdata" OPTS="-r $(REP_CNT) -o ./assets/clang_array_unroll_O2$(ANALYZE_NUM).txt  -c $(MEASURE_CNT)" rebuild all ;
	sudo rm ./default.profraw ;
	\
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                      -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_O2$(ANALYZE_NUM).txt       	  	   -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=0 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings.txt\"'                      -ffast-math -funroll-loops -flto -mfma -fprofile-use"      OPTS="-r $(REP_CNT) -o ./assets/gcc_O2$(ANALYZE_NUM).txt       		   -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86           -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_O2$(ANALYZE_NUM).txt  		   -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86           -ffast-math -funroll-loops -flto -mfma -fprofile-use"   	  OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_O2$(ANALYZE_NUM).txt  		   -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL  -ffast-math -funroll-loops -flto -mfma -fprofile-generate" OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_unroll_O2$(ANALYZE_NUM).txt     -c $(MEASURE_CNT)" rebuild all ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  -DX86 -DUNROLL  -ffast-math -funroll-loops -flto -mfma -fprofile-use" 	  OPTS="-r $(REP_CNT) -o ./assets/gcc_avx2_unroll_O2$(ANALYZE_NUM).txt     -c $(MEASURE_CNT)" rebuild all ;
	make clean_gcda ;
	sudo nice -n -20 make COMPILER=gcc DEBUG_=0 USE_AVX2=1 OPTIMIZE_LVL=-O2 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"'  			       -ffast-math -funroll-loops -flto -mfma" 		              OPTS="-r $(REP_CNT) -o ./assets/gcc_array_unroll_O2$(ANALYZE_NUM).txt    -c $(MEASURE_CNT)" rebuild all ;

analyze:
	python $(SRC_DIR)/analyze.py $(MEASURE_CNT) $(REP_CNT) $(OUTPUTS_NUM) $(GRAPHIC_NUM)

stat_check:
	python $(SRC_DIR)/stat_check.py $(OUTPUTS_NUM) $(STAT_CHECK_NUM)

# make USE_AVX2=1 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"'" 			OPTS="-g" DEBUG_=0 rebuild all 
# make USE_AVX2=1 ADD_FLAGS="-DSETTINGS_FILENAME='\"settings_avx.txt\"' -DX86" 	    OPTS="-g" DEBUG_=0 rebuild all
# make USE_AVX2=0 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings.txt\"'"     			OPTS="-g" DEBUG_=0 rebuild all 


# make USE_AVX2=0 OPTIMIZE_LVL=-O0 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings.txt\"'"  OPTS="-g" DEBUG_=0 rebuild all

# make USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings.txt\"' -ffast-math -funroll-loops -flto -mfma -fprofile-generate"  OPTS="-g" DEBUG_=0 rebuild all 
# make USE_AVX2=0 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings.txt\"' -ffast-math -funroll-loops -flto -mfma -fprofile-use"  OPTS="-g" DEBUG_=0 rebuild all 

# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -DX86 -ffast-math -funroll-loops -flto -mfma -fprofile-generate"  OPTS="-g" DEBUG_=0 rebuild all 
# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -DX86 -ffast-math -funroll-loops -flto -mfma -fprofile-use"  OPTS="-g" DEBUG_=0 rebuild all

# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-generate"  OPTS="-g" DEBUG_=0 rebuild all 
# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -DX86 -DUNROLL -ffast-math -funroll-loops -flto -mfma -fprofile-use"  OPTS="-g" DEBUG_=0 rebuild all

# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -ffast-math -funroll-loops -flto -mfma -fprofile-generate"  OPTS="-g" DEBUG_=0 rebuild all 
# make USE_AVX2=1 OPTIMIZE_LVL=-O3 ADD_FLAGS=-"DSETTINGS_FILENAME='\"settings_avx.txt\"' -ffast-math -funroll-loops -flto -mfma -fprofile-use"  OPTS="-g" DEBUG_=0 rebuild all

# python src/analyze.py 20 5 ./assets/gcc_O20.txt ./assets/gcc_avx2_O21.txt ./assets/gcc_avx2_O31.txt  ./assets/gcc_avx2_unroll_O21.txt ./assets/gcc_avx2_unroll_O31.txt ./assets/gcc_array_unroll_O21.txt ./assets/gcc_array_unroll_O31.txt ./assets/clang_avx2_O21.txt ./assets/clang_avx2_O31.txt ./assets/clang_avx2_unroll_O21.txt ./assets/clang_avx2_unroll_O31.txt ./assets/clang_array_unroll_O21.txt ./assets/clang_array_unroll_O31.txt assets/histogram1.png
