# List of all the hydranfc related files.
HYDRANFCSRC = hydranfc/hydranfc.c \
              hydranfc/hydranfc_cmd_sniff.c \
              hydranfc/hydranfc_cmd_sniff_downsampling.c \
              hydranfc/hydranfc_cmd_sniff_iso14443.c \
              hydranfc/hydranfc_emul_14443a_sdd.c \
              hydranfc/hydranfc_emul_mifare.c \
              hydranfc/file_fmt_pcap.c \
              hydranfc/hydranfc_emul_mf_ultralight.c \
              hydranfc/hydranfc_bbio_reader.c

# Required include directories
HYDRANFCINC = ./hydranfc
