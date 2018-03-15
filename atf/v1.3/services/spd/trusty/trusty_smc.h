#define SMC_NR(entity, fn, fastcall, smc64) ((((fastcall) & 0x1) << 31) | \
					     (((smc64) & 0x1) << 30) | \
					     (((entity) & 0x3F) << 24) | \
					     ((fn) & 0xFFFF) \
					    )

#define SMC_FASTCALL_NR(entity, fn)	SMC_NR((entity), (fn), 1, 0)
#define SMC_STDCALL_NR(entity, fn)	SMC_NR((entity), (fn), 0, 0)

#define SMC_FASTCALL64_NR(entity, fn)	SMC_NR((entity), (fn), 1, 1)
#define SMC_STDCALL64_NR(entity, fn)	SMC_NR((entity), (fn), 0, 1)

#define	SMC_ENTITY_ARCH			0	/* ARM Architecture calls */
#define	SMC_ENTITY_CPU			1	/* CPU Service calls */
#define	SMC_ENTITY_SIP			2	/* SIP Service calls */
#define	SMC_ENTITY_OEM			3	/* OEM Service calls */
#define	SMC_ENTITY_STD			4	/* Standard Service calls */
#define	SMC_ENTITY_RESERVED		5	/* Reserved for future use */
#define	SMC_ENTITY_TRUSTED_APP		48	/* Trusted Application calls */
#define	SMC_ENTITY_TRUSTED_OS		50	/* Trusted OS calls */
#define SMC_ENTITY_LOGGING              51	/* Used for secure -> nonsecure logging */
#define	SMC_ENTITY_SECURE_MONITOR	60	/* Trusted OS calls internal to secure monitor */

/* FC = Fast call, SC = Standard call */

#define SMC_FC_CPU_SUSPEND	SMC_FASTCALL_NR (SMC_ENTITY_SECURE_MONITOR, 7)
#define SMC_FC_CPU_RESUME	SMC_FASTCALL_NR (SMC_ENTITY_SECURE_MONITOR, 8)

#define SMC_SC_MTK_LEGACY_TA         SMC_STDCALL_NR  (SMC_ENTITY_TRUSTED_OS, 8)
#define SMC_SC_MTK_LEGACY_TA_DONE    SMC_STDCALL_NR  (SMC_ENTITY_TRUSTED_OS, 9)

/* TRUSTED_OS entity calls */
#define SMC_SC_VIRTIO_GET_DESCR SMC_STDCALL_NR(SMC_ENTITY_TRUSTED_OS, 20)
#define SMC_SC_VIRTIO_START SMC_STDCALL_NR(SMC_ENTITY_TRUSTED_OS, 21)
#define SMC_SC_VIRTIO_STOP  SMC_STDCALL_NR(SMC_ENTITY_TRUSTED_OS, 22)

#define SMC_SC_VDEV_RESET   SMC_STDCALL_NR(SMC_ENTITY_TRUSTED_OS, 23)
#define SMC_SC_VDEV_KICK_VQ SMC_STDCALL_NR(SMC_ENTITY_TRUSTED_OS, 24)

#define SMC_SC_NS_RETURN    SMC_STDCALL_NR  (SMC_ENTITY_SECURE_MONITOR, 0)


#define SMC_FC_RD_CNPTS_CTL    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 64)
#define SMC_FC_WR_CNPTS_CTL    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 65)

//#define SMC_FC_RD_CNPTS_CVA    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 66)
#define SMC_FC_WR_CNPTS_CVA    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 67)

//#define SMC_FC_RD_CNPTS_TVA    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 68)
#define SMC_FC_WR_CNPTS_TVA    SMC_FASTCALL_NR(SMC_ENTITY_SECURE_MONITOR, 69)

