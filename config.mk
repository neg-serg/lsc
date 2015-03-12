CC = gcc
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro
CFLAGS = -march=native -O2 -fPIE -pie -pipe -D_FORTIFY_SOURCE=1 -fstack-protector-strong --param=ssp-buffer-size=4 
