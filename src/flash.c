#include "flash.h"

#include <stdlib.h>

#include "nrfx_qspi.h"

#include "app_error.h"
#include "boards.h"

#define NRF_LOG_MODULE_NAME flash
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

volatile bool m_finished = false;

#define QSPI_TEST_DATA_SIZE 256

static uint8_t m_buffer_tx[QSPI_TEST_DATA_SIZE];
static uint8_t m_buffer_rx[QSPI_TEST_DATA_SIZE];

#define WAIT_FOR_PERIPH()   \
    do                      \
    {                       \
        while (!m_finished) \
        {                   \
        }                   \
        m_finished = false; \
    } while (0)

static void qspi_handler(nrfx_qspi_evt_t event, void *p_context)
{
    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(p_context);

    NRF_LOG_INFO("event complete");
    m_finished = true;
}

static int configure_memory()
{
    ret_code_t err_code;
    uint8_t temporary = 0x40;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode = QSPI_STD_CMD_RSTEN,
        .length = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait = true,
        .wren = true};

    // Send reset enable
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Send reset command
    cinstr_cfg.opcode = QSPI_STD_CMD_RST;
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Switch to qspi mode
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
    err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, &temporary, NULL);
    APP_ERROR_CHECK(err_code);

    return 0;
}

static int wake()
{
    const nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode = QSPI_MX25_CMD_RDP,
        .length = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait = true,
        .wren = true};

    // Wake
    ret_code_t err_code = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    return 0;
}

static int test()
{

    ret_code_t err_code;

    srand(0);
    for (int i = 0; i < QSPI_TEST_DATA_SIZE; ++i)
    {
        m_buffer_tx[i] = (uint8_t)rand();
    }

    err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, 0);
    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Process of erasing first block start");

    err_code = nrfx_qspi_write(m_buffer_tx, QSPI_TEST_DATA_SIZE, 0);
    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Process of writing data start");

    err_code = nrfx_qspi_read(m_buffer_rx, QSPI_TEST_DATA_SIZE, 0);
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Data read");

    NRF_LOG_INFO("Compare...");
    if (memcmp(m_buffer_tx, m_buffer_rx, QSPI_TEST_DATA_SIZE) == 0)
    {
        NRF_LOG_INFO("Data consistent");
    }
    else
    {
        NRF_LOG_INFO("Data inconsistent");
    }

    // nrfx_qspi_uninit();

    return 0;
}

void flash_init()
{
    // qspi configuration
    nrfx_qspi_config_t config = {
        .xip_offset = NRFX_QSPI_CONFIG_XIP_OFFSET,
        .pins = {
            .sck_pin = F_CLK,
            .csn_pin = F_CS,
            .io0_pin = F_SI,
            .io1_pin = F_SO,
            .io2_pin = F_WP,
            .io3_pin = F_HOLD,
        },
        .irq_priority = NRFX_QSPI_FLASH_IRQ_PRIORITY,
        .prot_if = {
            .readoc = NRF_QSPI_READOC_READ4IO,
            .writeoc = NRF_QSPI_WRITEOC_PP4IO,
            .addrmode = NRF_QSPI_ADDRMODE_24BIT,
            .dpmconfig = false,
        },
        .phy_if = {
            .sck_freq = NRF_QSPI_FREQ_32MDIV1,
            .sck_delay = NRFX_QSPI_CONFIG_SCK_DELAY,
            .spi_mode = NRF_QSPI_MODE_0,
            .dpmen = false,
        },
    };

    // Init qspi
    ret_code_t err_code = nrfx_qspi_init(&config, qspi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    // Configure memory.
    configure_memory();

    UNUSED_VARIABLE(m_buffer_rx);
    UNUSED_VARIABLE(m_buffer_tx);

    // Wake up?
    wake();

    // Do a test;
    // UNUSED_VARIABLE(test());
    NRF_LOG_INFO("%x", NRF_QSPI->IFCONFIG1);
    test();
    NRF_LOG_INFO("%x", NRF_QSPI->IFCONFIG1);
    // test();
}

// TODO: handling reading and writing > buffer size
int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    ret_code_t err_code;

    NRF_LOG_INFO("flash read %d offset %d", block, off);

    // Calculate the  max size/address that is accessible
    const uint32_t max_size = c->block_count * c->block_size;

    // Check to make sure the block is correct.
    if (block > c->block_count ||
        (block * c->block_size + off) > max_size)
    {
        APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
    }

    // Calculate the memory address by multiplying the blocks and adding the offset.
    uint32_t addr = block * c->block_size + off;

    // Read the data
    err_code = nrfx_qspi_read(buffer, size, addr);
    APP_ERROR_CHECK(err_code);

    // TODO: kill this.
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Data read");

    return 0;
}

// TODO: handling reading and writing > buffer size
int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{

    ret_code_t err_code;

    NRF_LOG_INFO("flash prog %d offset %d", block, off);

    // Calculate the  max size/address that is accessible
    const uint32_t max_size = c->block_count * c->block_size;

    // Check to make sure the block is correct.
    if (block > c->block_count ||
        (block * c->block_size + off) > max_size)
    {
        APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
    }

    // Calculate the memory address by multiplying the blocks and adding the offset.
    uint32_t addr = block * c->block_size + off;

    // Read the data
    err_code = nrfx_qspi_write(buffer, size, addr);
    APP_ERROR_CHECK(err_code);

    // TODO: kill this.
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Data written");

    return 0;
}

int flash_erase(const struct lfs_config *c, lfs_block_t block)
{

    ret_code_t err_code;

    NRF_LOG_INFO("erase block %d", block);

    // Check to make sure the block is correct.
    if (block > c->block_count)
    {
        APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
    }

    // Calculate the memory address by multiplying the blocks and adding the offset.
    uint32_t addr = block * c->block_size;

    // Erase at a certain address
    err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, addr);
    APP_ERROR_CHECK(err_code);

    // TODO: kill this.
    WAIT_FOR_PERIPH();
    NRF_LOG_INFO("Process of erasing first block start");

    return 0;
}

int flash_sync(const struct lfs_config *c)
{
    return 0;
}