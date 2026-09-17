#[cfg(not(feature = "embassy"))]
use esp_hal::{
    delay::Delay,
    gpio::{Level, Output, OutputConfig},
    peripherals::{GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, SPI2},
    spi::{
        Mode,
        master::{Config, Spi},
    },
    time::Rate,
};
#[cfg(not(feature = "embassy"))]
use shrike_core::{error::FlashError, universal::UniversalSPIflash};

/*
  #define SHRIKE_DEFAULT_EN_PIN     9
  #define SHRIKE_DEFAULT_PWR_PIN    8
  #define SHRIKE_DEFAULT_SS_PIN    10
  #define SHRIKE_DEFAULT_SCK_PIN   12
  #define SHRIKE_DEFAULT_MOSI_PIN  11
  #define SHRIKE_DEFAULT_MISO_PIN  13
*/

#[cfg(not(feature = "embassy"))]
pub fn init_esp32s3_normal(
    spi: SPI2<'static>,
    sclk: GPIO12<'static>,
    mosi: GPIO11<'static>,
    miso: GPIO13<'static>,
    ss: GPIO10<'static>,
    en: GPIO9<'static>,
    pwr: GPIO8<'static>,
    bin: &[u8],
) -> Result<(), FlashError> {
    use shrike_core::universal::Flash;

    let config = Config::default()
        .with_frequency(Rate::from_hz(1_600_000))
        .with_mode(Mode::_0);

    let spi = Spi::new(spi, config)
        .map_err(|_| FlashError::SpiWriteError)?
        .with_sck(sclk)
        .with_mosi(mosi)
        .with_miso(miso);

    let en = Output::new(en, Level::Low, OutputConfig::default());
    let ss = Output::new(ss, Level::Low, OutputConfig::default());
    let pwr = Output::new(pwr, Level::Low, OutputConfig::default());

    let mut delay = Delay::new();
    let mut flash = UniversalSPIflash::new(spi, en, pwr, ss, &mut delay);
    flash.flash(bin)
}

#[cfg(feature = "embassy")]
pub mod async_esp {
    use esp_hal::{
        dma::{DmaRxBuf, DmaTxBuf},
        gpio::{Level, Output, OutputConfig},
        peripherals::{DMA_CH0, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, SPI2},
        spi::{
            Mode,
            master::{Config, Spi},
        },
        time::Rate,
    };
    use shrike_core::{
        async_universal::{AsyncFlash, UniversalAsyncSPIflash},
        error::FlashError,
        universal::UniversalSPIflash,
    };

    pub async fn init_esp32s3_blocking_async(
        spi: SPI2<'static>,
        sclk: GPIO12<'static>,
        mosi: GPIO11<'static>,
        miso: GPIO13<'static>,
        ss: GPIO10<'static>,
        en: GPIO9<'static>,
        pwr: GPIO8<'static>,
        bin: &[u8],
    ) -> Result<(), FlashError> {
        let config = Config::default()
            .with_frequency(Rate::from_hz(1_600_000))
            .with_mode(Mode::_0);

        let spi = Spi::new(spi, config)
            .map_err(|_| FlashError::SpiWriteError)?
            .with_sck(sclk)
            .with_mosi(mosi)
            .with_miso(miso);

        let en = Output::new(en, Level::Low, OutputConfig::default());
        let ss = Output::new(ss, Level::Low, OutputConfig::default());
        let pwr = Output::new(pwr, Level::Low, OutputConfig::default());

        let mut flash = UniversalSPIflash::new(spi, en, pwr, ss);
        flash.flash(bin).await
    }

    pub async fn init_esp32s3_all_async<'d>(
        spi: SPI2<'static>,
        sclk: GPIO12<'static>,
        mosi: GPIO11<'static>,
        miso: GPIO13<'static>,
        ss: GPIO10<'static>,
        en: GPIO9<'static>,
        pwr: GPIO8<'static>,
        dma_channel: DMA_CH0<'static>,
        rx_buf: DmaRxBuf,
        tx_buf: DmaTxBuf,
        bin: &[u8],
    ) -> Result<(), FlashError> {
        let config = Config::default()
            .with_frequency(Rate::from_hz(1_600_000))
            .with_mode(Mode::_0);

        let spi = Spi::new(spi, config)
            .map_err(|_| FlashError::SpiWriteError)?
            .with_sck(sclk)
            .with_mosi(mosi)
            .with_miso(miso)
            .with_dma(dma_channel)
            .with_buffers(rx_buf, tx_buf)
            .into_async();

        let en = Output::new(en, Level::Low, OutputConfig::default());
        let ss = Output::new(ss, Level::Low, OutputConfig::default());
        let pwr = Output::new(pwr, Level::Low, OutputConfig::default());

        let mut flash = UniversalAsyncSPIflash::new(spi, en, pwr, ss);
        flash.flash(bin).await
    }
}
