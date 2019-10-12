# List of all the hydranfc related files.
HYDRANFCSRC = hydranfc/hydranfc.c \
              hydranfc/hydranfc_cmd_sniff.c \
              hydranfc/hydranfc_cmd_sniff_downsampling.c \
              hydranfc/hydranfc_cmd_sniff_iso14443.c \
              hydranfc_emul_14443a_sdd.c \
              hydranfc_emul_mifare.c \
              file_fmt_pcap.c \
              hydranfc_emul_mf_ultralight.c

# Required include directories
HYDRANFCINC = ./hydranfc \
              ./hydranfc/low_level
