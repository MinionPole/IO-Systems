#include <linux/module.h>
#include <linux/fs.h>		
#include <linux/errno.h>	
#include <linux/types.h>	
#include <linux/fcntl.h>	
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/string.h>


/* Variable for Major Number */
int c = 0;

#define SECTOR_SIZE 512  /* bytes */
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 
#define PARTITION_TABLE_SIZE 64 
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55

typedef struct
{

    /* 0x00 - Inactive; 0x80 - Active (Bootable) */
    unsigned char boot_type;

    /* CHS of part first sector, sectors enumerated from one */
    /* https://ru.wikipedia.org/wiki/CHS */
    unsigned char start_head;
    unsigned char start_sec:6;
    unsigned char start_cyl_hi:2;
    unsigned char start_cyl;

    /* 0x83 - primary part, 0x05 - extended part */
    unsigned char part_type;

    /* CHS of part last sector, sectors enumerated from one */
    unsigned char end_head;
    unsigned char end_sec:6;
    unsigned char end_cyl_hi:2;
    unsigned char end_cyl;

    /* LBA of part first sector */
    /* https://ru.wikipedia.org/wiki/LBA */
    unsigned int abs_start_sec;

    /* count of sectors in part */
    unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

#define SEC_PER_HEAD 63
#define HEAD_PER_CYL 255
#define HEAD_SIZE (SEC_PER_HEAD * SECTOR_SIZE)
#define CYL_SIZE (SEC_PER_HEAD * HEAD_PER_CYL * SECTOR_SIZE)

#define sec4size(s) ((((s) % CYL_SIZE) % HEAD_SIZE) / SECTOR_SIZE)
#define head4size(s) (((s) % CYL_SIZE) / HEAD_SIZE)
#define cyl4size(s) ((s) / CYL_SIZE)

#define MB2SEC(mb) (mb * 1024 * 1024 / SECTOR_SIZE)

#define PRM 0x83
#define EXT 0x05

/****************************************************************************************
* LBA -> CHS & CHS -> LBA
* _______________________________________________________________________________________
*
* LBA = (C × HPC + H) × SPT + (S − 1)
*
* C = LBA ÷ (HPC × SPT)
* H = (LBA ÷ SPT) mod HPC
* S = (LBA mod SPT) + 1
* _______________________________________________________________________________________
* LBA is the logical block address
* C, H and S are the cylinder number, the head number, and the sector number
* HPC is the maximum number of heads per cylinder
* SPT is the maximum number of sectors per track
* _______________________________________________________________________________________
* https://en.wikipedia.org/wiki/Logical_block_addressing
*****************************************************************************************/

#define HPC HEAD_PER_CYL
#define SPT SEC_PER_HEAD

#define lba2cyl(lba) ((unsigned char)(lba / (HPC * SPT)))
#define lba2head(lba) ((unsigned char)((lba / SPT) % HPC))
#define lba2sec(lba) ((unsigned char)((lba % SPT) + 1))

/****************************************************************************************
 *                           PARTITIONS
 *                               50  <----- MEMSIZE
 *                               / \
 *                PRT1_1 -----> 2 + 48 <----- PRT1_2
 *                                  /|\
 *                 PRT2_1 -----> 10 18 20 <----- PRT2_3
 *                                   ⋀
 *                        PRT2_3 ____/
 *
*****************************************************************************************/

#define SEC_START 0x1

/*  Size of Ram disk in sectors */
#define MEMSIZE MB2SEC(50) 

/* Sizes of logical partitions */
#define PRT1_1 MB2SEC(2)
#define PRT2_1 MB2SEC(10)
#define PRT2_2 MB2SEC(18)
#define PRT2_3 MB2SEC(20)

/* Sizes of extended partitions */
#define EXTP_1 PRT2_1 + PRT2_2 + PRT2_3
#define EXTP_2_1 PRT2_2 + PRT2_3
#define EXTP_2_2 PRT2_3



static PartTable def_part_table =
{
    {
        boot_type: 0x00,
        start_sec: (lba2sec(SEC_START) & 0x3F),
        start_head: lba2head(SEC_START),
        start_cyl: (lba2cyl(SEC_START) & 0xFF),
        start_cyl_hi: (lba2cyl(SEC_START) & 0x300),
        part_type: PRM,
        end_head: lba2sec(SEC_START + PRT1_1 - 1),
        end_sec: lba2head(SEC_START + PRT1_1 - 1) & 0x3F,
        end_cyl: (lba2cyl(SEC_START + PRT1_1 - 1) & 0xFF),
        end_cyl_hi: (lba2cyl(SEC_START + PRT1_1 - 1) & 0x300),
        abs_start_sec: SEC_START,
        sec_in_part: PRT1_1 // 2Mbyte
    },
    {
        boot_type: 0x00,
        start_head: lba2head(SEC_START + PRT1_1),
        start_sec: lba2sec(SEC_START + PRT1_1) & 0x3F,
        start_cyl: lba2cyl(SEC_START + PRT1_1) & 0xFF,
        start_cyl_hi: lba2cyl(SEC_START + PRT1_1) & 0x300,
        part_type: EXT, // extended partition type
        end_sec: lba2sec(SEC_START + PRT1_1 + EXTP_1 - 1),
        end_head: lba2head(SEC_START + PRT1_1 + EXTP_1 - 1),
        end_cyl: lba2cyl(SEC_START + PRT1_1 + EXTP_1 - 1) & 0xFF,
        end_cyl_hi: lba2cyl(SEC_START + PRT1_1 + EXTP_1 - 1) & 0x300,
        abs_start_sec: PRT1_1 + 1,
        sec_in_part: EXTP_1 // 48 Mbyte
    }
};
static unsigned int def_log_part_br_abs_start_sector[] = {(PRT1_1 + 1), (PRT1_1 + PRT2_1 + 1), (PRT1_1 + PRT2_1 + PRT2_2 + 1)};
static const PartTable def_log_part_table[] =
{
    {
        {
            boot_type: 0x00,
            start_sec: lba2sec(SEC_START)  & 0x3F,
            start_head: lba2head(SEC_START),
            start_cyl: lba2cyl(SEC_START) & 0xFF,
            start_cyl_hi: lba2cyl(SEC_START) & 0x300,
            part_type: PRM,
            end_head: lba2sec(SEC_START + PRT2_1 - 1),
            end_sec: lba2head(SEC_START + PRT2_1 - 1)  & 0x3F,
            end_cyl: lba2cyl(SEC_START + PRT2_1 - 1) & 0xFF,
            end_cyl_hi: lba2cyl(SEC_START + PRT2_1 - 1) & 0x300,
            abs_start_sec: SEC_START,
            sec_in_part: PRT2_1 // 10 Mb
        },
        {
            boot_type: 0x00,
            start_head: lba2head(SEC_START + PRT2_1),
            start_sec: lba2sec(SEC_START + PRT2_1)  & 0x3F,
            start_cyl: lba2cyl(SEC_START + PRT2_1) & 0xFF,
            start_cyl_hi: lba2cyl(SEC_START + PRT2_1) & 0x300,
            part_type: EXT,
            end_head: lba2head(SEC_START + PRT2_1 + EXTP_2_1 - 1),
            end_sec: lba2head(SEC_START + PRT2_1 + EXTP_2_1 - 1)  & 0x3F,
            end_cyl: lba2cyl(SEC_START + PRT2_1 + EXTP_2_1 - 1) & 0xFF,
            end_cyl_hi: lba2cyl(SEC_START + PRT2_1 + EXTP_2_1 - 1) & 0x300,
            abs_start_sec: PRT2_1,
            sec_in_part: PRT2_2 + 1  // 38 Mb
        }
    },
    {
        {
            boot_type: 0x00,
            start_sec: lba2sec(SEC_START) & 0x3F,
            start_head: lba2head(SEC_START),
            start_cyl: lba2cyl(SEC_START) & 0xFF,
            start_cyl_hi: lba2cyl(SEC_START) & 0x300,
            part_type: PRM,
            end_head: lba2sec(SEC_START + PRT2_2 - 1),
            end_sec: lba2head(SEC_START + PRT2_2 - 1) & 0x3F,
            end_cyl: lba2cyl(SEC_START + PRT2_2 - 1) & 0xFF,
            end_cyl_hi: lba2cyl(SEC_START + PRT2_2 - 1) & 0x300,
            abs_start_sec: SEC_START,
            sec_in_part: PRT2_2 // 18 Mb
        },
        {
            boot_type: 0x00,
            start_head: lba2head(SEC_START + PRT2_2),
            start_sec: lba2sec(SEC_START + PRT2_2) & 0x3F,
            start_cyl: lba2cyl(SEC_START + PRT2_2) & 0xFF,
            start_cyl_hi: lba2cyl(SEC_START + PRT2_2) & 0x300,
            part_type: EXT,
            end_head: lba2head(SEC_START + PRT2_2 + EXTP_2_2 - 1),
            end_sec: lba2head(SEC_START + PRT2_2 + EXTP_2_2 - 1) & 0x3F,
            end_cyl: lba2cyl(SEC_START + PRT2_2 + EXTP_2_2 - 1) & 0xFF,
            end_cyl_hi: lba2cyl(SEC_START + PRT2_2 + EXTP_2_2 - 1) & 0x300,
            abs_start_sec: PRT2_2 + PRT2_1,
            sec_in_part: PRT2_3 + 1 // 20 Mb
        }
    },
    {
        {
            boot_type: 0x00,
            start_sec: lba2sec(SEC_START) & 0x3F,
            start_head: lba2head(SEC_START),
            start_cyl: lba2cyl(SEC_START) & 0xFF,
            start_cyl_hi: lba2cyl(SEC_START) & 0x300,
            part_type: PRM,
            end_head: lba2sec(SEC_START + PRT2_3 - 1),
            end_sec: lba2head(SEC_START + PRT2_3 - 1) & 0x3F,
            end_cyl: lba2cyl(SEC_START + PRT2_3 - 1) & 0xFF,
            end_cyl_hi: lba2cyl(SEC_START + PRT2_3 - 1) & 0x300,
            abs_start_sec: SEC_START,
            sec_in_part: PRT2_3 // 20 Mb
        }
    }
};

static void copy_mbr(u8 *disk)
{
    memset(disk, 0x0, MBR_SIZE);
    *(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = 0x36E5756D;
    memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
    *(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
}
static void copy_br(u8 *disk, int abs_start_sector, const PartTable *part_table)
{
    disk += (abs_start_sector * SECTOR_SIZE);
    memset(disk, 0x0, BR_SIZE);
    memcpy(disk + PARTITION_TABLE_OFFSET, part_table,
        PARTITION_TABLE_SIZE);
    *(unsigned short *)(disk + BR_SIGNATURE_OFFSET) = BR_SIGNATURE;
}
void copy_mbr_n_br(u8 *disk)
{
    int i;

    copy_mbr(disk);
    for (i = 0; i < ARRAY_SIZE(def_log_part_table); i++)
    {
        copy_br(disk, def_log_part_br_abs_start_sector[i], &def_log_part_table[i]);
    }
}
/* Structure associated with Block device */
struct mydiskdrive_dev 
{
    int size;
    u8 *data;
    spinlock_t lock;
    struct request_queue *queue;
    struct gendisk *gd;

}device;

struct mydiskdrive_dev *x;

static int my_open(struct block_device *x, fmode_t mode)	 
{
    int ret=0;
    printk(KERN_INFO "mydiskdrive : open \n");

    return ret;

}

static void my_release(struct gendisk *disk, fmode_t mode)
{
    printk(KERN_INFO "mydiskdrive : closed \n");
}

static struct block_device_operations fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
};

int mydisk_init(void)
{
    (device.data) = vmalloc(MEMSIZE * SECTOR_SIZE);
    /* Setup its partition table */
    copy_mbr_n_br(device.data);

    return MEMSIZE;	
}

static int rb_transfer(struct request *req)
{
    int dir = rq_data_dir(req);
    int ret = 0;
    /*starting sector
     *where to do operation*/
    sector_t start_sector = blk_rq_pos(req);
    unsigned int sector_cnt = blk_rq_sectors(req); /* no of sector on which opn to be done*/
    struct bio_vec bv;
    #define BV_PAGE(bv) ((bv).bv_page)
    #define BV_OFFSET(bv) ((bv).bv_offset)
    #define BV_LEN(bv) ((bv).bv_len)
    struct req_iterator iter;
    sector_t sector_offset;
    unsigned int sectors;
    u8 *buffer;
    sector_offset = 0;
    rq_for_each_segment(bv, req, iter)
    {
        buffer = page_address(BV_PAGE(bv)) + BV_OFFSET(bv);
        if (BV_LEN(bv) % (SECTOR_SIZE) != 0)
        {
            printk(KERN_ERR"bio size is not a multiple ofsector size\n");
            ret = -EIO;
        }
        sectors = BV_LEN(bv) / SECTOR_SIZE;
        // printk(KERN_DEBUG "my disk: Start Sector: %llu, Sector Offset: %llu;
        // Buffer: %p; Length: %u sectors\n",
        // (unsigned long long)(start_sector), (unsigned long long) 
        // (sector_offset), buffer, sectors);

        if (dir == WRITE) /* Write to the device */
        {
            u8 *device_data = (device.data) + ((start_sector + sector_offset) * SECTOR_SIZE);
            int i;
            for (i = 0; i < sectors * SECTOR_SIZE; i++) {
                if(device_data[i] != buffer[i]) {
                    // if(buffer[i] == 0xa){
                    // 	buffer[i] = device_data[i]
                    // 	continue;
                    // }
                    pr_debug("Writing");
                    if (i < 3)
                    {
                        /* Copy the first three bytes unchanged */
                        pr_info("Writing first 3 bytes: %hhx", buffer[i]);
                    } 
                    else 
                    {
                        /* Calculate the new byte as the arithmetic average of the three previous bytes */
                        buffer[i] = DIV_ROUND_CLOSEST(buffer[i-3] + buffer[i-2] + buffer[i-1], 3);
                        pr_info("Writing average 3 bytes: avg(%hhx,%hhx,%hhx) = %hhx", buffer[i-3], buffer[i-2], buffer[i-1], buffer[i]);
                    }
                }
            }
            memcpy(device_data\
            ,buffer,sectors*SECTOR_SIZE);		
        }
        else /* Read from the device */
        {
            memcpy(buffer,(device.data)+((start_sector+sector_offset)\
            *SECTOR_SIZE),sectors*SECTOR_SIZE);	
        }
        sector_offset += sectors;
    }
    
    if (sector_offset != sector_cnt)
    {
        printk(KERN_ERR "mydisk: bio info doesn't match with the request info");
        ret = -EIO;
    }
    return ret;
}
/** request handling function**/
static void dev_request(struct request_queue *q)
{
    struct request *req;
    int error;
    while ((req = blk_fetch_request(q)) != NULL) /*check active request 
                              *for data transfer*/
    {
    error=rb_transfer(req);// transfer the request for operation
        __blk_end_request_all(req, error); // end the request
    }
}

void device_setup(void)
{
    mydisk_init();
    c = register_blkdev(c, "mydisk");// major no. allocation
    printk(KERN_ALERT "Major Number is : %d",c);
    spin_lock_init(&device.lock); // lock for queue
    device.queue = blk_init_queue( dev_request, &device.lock); 

    device.gd = alloc_disk(8); // gendisk allocation
    
    (device.gd)->major=c; // major no to gendisk
    device.gd->first_minor=0; // first minor of gendisk

    device.gd->fops = &fops;
    device.gd->private_data = &device;
    device.gd->queue = device.queue;
    device.size= mydisk_init();
    printk(KERN_INFO"THIS IS DEVICE SIZE %d",device.size);	
    sprintf(((device.gd)->disk_name), "mydisk");
    set_capacity(device.gd, device.size);  
    add_disk(device.gd);
}

static int __init mydiskdrive_init(void)
{	
    int ret=0;
    device_setup();
    
    return ret;
}

void mydisk_cleanup(void)
{
    vfree(device.data);
}

void __exit mydiskdrive_exit(void)
{
    del_gendisk(device.gd);
    put_disk(device.gd);
    blk_cleanup_queue(device.queue);
    unregister_blkdev(c, "mydisk");
    mydisk_cleanup();	
}

module_init(mydiskdrive_init);
module_exit(mydiskdrive_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("BLOCK DRIVER");
