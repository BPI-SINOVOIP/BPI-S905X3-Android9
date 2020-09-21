#****************************************************************************
# 通用的生成动态库�?c/c++ GNU Makefile 
# Writen By ZhuFeng,2007.2
# 功能：自动从所有的.c�?cpp文件列表中生成输出文件，可以生成动态库
#****************************************************************************
   
#常用定义选项 
DW_NAME		= faad
DW_VERSION		= 1.1.0
MACHINE		= $(shell uname -m)
OUTPUT_BIN_PATH		= ../bin/
OUTPUT_INCLUDE_PATH	= ./include/
SRC_PATH		= ./

SRC_FILES		=  $(wildcard $(SRC_PATH)*.c)
		
EXPORT_INCLUDE_FILES	= $(wildcard $(SRC_PATH)*.h) $(wildcard $(SRC_PATH)codebook/*.h)
 
USING_LIBS		= 
USING_INCLUDES_PATH	= 

#将会自动附加$(MACHINE)/$DEBUG
USING_LIBS_PATH		= 
DEBUG  		= YES
DEFINES		= 

#通用定义
G++		= g++
GCC		= gcc
LD 		= g++

DEBUG_CFLAGS     := -c -Wall -shared -fPIC -Wno-format -g -DDEBUG -fexceptions -fnon-call-exceptions
RELEASE_CFLAGS   := -c -Wall -shared -fPIC -Wno-unknown-pragmas -Wno-format -O3 -fexceptions -fnon-call-exceptions

DEBUG_LDFLAGS    := -g -shared -fPIC -Wl,-soname,lib$(DW_NAME).so
RELEASE_LDFLAGS  := -shared -fPIC -Wl,-soname,lib$(DW_NAME).so


ifeq (YES, ${DEBUG})
	CFLAGS       	:= ${DEBUG_CFLAGS}
	LDFLAGS      	:= ${DEBUG_LDFLAGS}
	USING_LIBS_PATH	:= $(addsuffix $(MACHINE)/debug/,$(USING_LIBS_PATH))
	OUTPUT_PATH	:= $(OUTPUT_BIN_PATH)
else
	CFLAGS      	:= ${RELEASE_CFLAGS}
	LDFLAGS      	:= ${RELEASE_LDFLAGS}
	USING_LIBS_PATH	:= $(addsuffix $(MACHINE)/release/,$(USING_LIBS_PATH))
	OUTPUT_PATH	:= $(OUTPUT_BIN_PATH)$(MACHINE)/release/
endif

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS		+= $(DEFINES)
CFLAGS		+= $(USING_INCLUDES_PATH)

#****************************************************************************
# 定义自动化编译宏，使用GCC方式
#****************************************************************************
# $(call make-depend,source-file,object-file,depend-file)
define make-depend
	$(GCC) -MM -MF $3 -MP -MT $2 $(CFLAGS) $1
endef

OBJS 		= $(addsuffix .o,$(basename $(SRC_FILES)))
DEPENDS 		= $(addsuffix .d,$(basename $(SRC_FILES)))

DW_SVNVER			= $(shell svnversion . | sed "s/.*://") 
DW_BUILD_DATE		= $(shell date)

OUTPUT 			= $(OUTPUT_PATH)lib$(DW_NAME).so.$(DW_VERSION)
CFLAGS		+= -DDW_SVNVER="\"$(DW_SVNVER)\"" -DDW_BUILD_DATE="\"$(DW_BUILD_DATE)\"" -DDW_VERSION="\"$(DW_VERSION)\""

VPATH		= $(SRC_PATH)
EXPORT_INCLUDE_FILES := $(addprefix $(SRC_PATH),$(EXPORT_INCLUDE_FILES))


default:$(OUTPUT)
	@echo "Build_Version: $(DW_VERSION)"
	@ln -sf ./lib$(DW_NAME).so.$(DW_VERSION) $(OUTPUT_PATH)lib$(DW_NAME).so
	@cp $(EXPORT_INCLUDE_FILES) $(OUTPUT_INCLUDE_PATH)
	@echo  "$(OUTPUT) MAKE 处理完毕"
	@echo  "\\033[32m\\033[1m[生成]\\033[21m 执行完毕!"
	@echo  "$(OUTPUT) 被生�?\\12$(EXPORT_INCLUDE_FILES)被复制到$(OUTPUT_INCLUDE_PATH)\\033[0m"

$(OUTPUT):$(OBJS)
	@mkdir -p $(OUTPUT_PATH)
	$(LD) $(LDFLAGS) -o $@ $^ $(USING_LIBS_PATH) $(USING_LIBS)
	@echo "\\033[35m\\033[1m[链接]\\033[21m $@ 执行完毕，总计更新处理�?(words $?) 个文件\\033[0m"

%.o:%.c
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@$(GCC) $(CFLAGS) $< -o $@
	@echo "\\033[32m\\033[1m[编译]\\033[21m $< 执行完毕\\033[0m"
	
%.o:%.cpp
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@$(G++) $(CFLAGS) $< -o $@
	@echo "\\033[32m\\033[1m[编译]\\033[21m $< 执行完毕\\033[0m"

-include $(DEPENDS)

help:
	@echo "\\033[1m\\033[33m============== 通用动态库MAKEFILE ==============\\033[0m"
	@echo "\\033[33m  2007.2 Writen By ZhuFeng \\033[0m"
	@echo "\\033[32m\\033[1mmake		:\\033[21m 缺省调用方式，使用debug模式进行编译与连接，输出二进制文件到指定目录\\033[0m"
	@echo "\\033[32m\\033[1mmake DEBUG=NO	:\\033[21m 使用release模式进行编译与连接，输出二进制文件到指定目录 \\033[0m"
	@echo "\\033[32m\\033[1mmake help	:\\033[21m显示帮助信息\\033[0m"
	@echo "\\033[32m\\033[1mmake rebuild	:\\033[21m重新生成所有文件\\033[0m"
	@echo "\\033[32m\\033[1mmake clean	:\\033[21m清除生成的中间文件\\033[0m"
	@echo "\\033[32m\\033[1mmake install	:\\033[21m安装生成的文件到/usr/local/lib目录，拷贝头文件�?usr/local/include目录\\033[0m"


rebuild:clean default
	@echo  "\\033[34m\\033[1m[rebuild]\\033[21m 执行完毕\\033[0m"
clean:
	@rm -f *.o
	@rm -f *.d
	@echo  "\\033[34m\\033[1m[clean]\\033[21m 执行完毕\\033[0m"
test:
	@echo "SVN_VERSION:$(SVN_VERSION)"
	@echo "DW_VERSION:$(DW_VERSION)"
	@echo "OUTPUT:$(OUTPUT)"






