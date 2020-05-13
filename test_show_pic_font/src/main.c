//This is a spi sample to initalize spi driver of nRF9160 board.
//No clock and slave select connection are required.
//Simple connect MOSI to MISO i.e. pin 11 to 12. A data loop back. 
//Data sent by the board is read by the board.

#include <nrf9160.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#ifdef CONFIG_SOC_NRF9160
#define SPI_DEV "SPI_3"
#else
#define SPI_DEV "SPI_1"
#endif

static const struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_CPOL | SPI_MODE_CPHA,
	.frequency = 4000000,
	.slave = 0,
};

struct device * spi_devl;

static void spi_init(void)
{
	spi_devl = device_get_binding(SPI_DEV);

	if (!SPI_DEV) {
		printk("Could not get %s device\n", SPI_DEV);
		return;
	}
}

void spi_test_send(void)
{
	int err;
	static u8_t tx_buffer[1];
	static u8_t rx_buffer[1];

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_transceive(spi_devl, &spi_cfg, &tx, &rx);
	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		printk("TX sent: %x\n", tx_buffer[0]);
		printk("RX recv: %x\n", rx_buffer[0]);
		tx_buffer[0]++;
	}	
}

void main(void)
{
	printk("SPIM Example\n");
	spi_init();

	while (1) 
	{
		spi_test_send();
		k_sleep(1000);
	}
}
