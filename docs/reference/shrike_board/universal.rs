use embedded_hal::{digital::OutputPin, spi::SpiBus};

#[cfg(feature = "embassy")]
use crate::async_universal::AsyncFlash;
use crate::error::FlashError;

#[cfg(not(feature = "embassy"))]
use embedded_hal::delay::DelayNs;

pub trait Flash {
    fn flash(&mut self, bin: &[u8]) -> Result<(), FlashError>;
}

// -------------------------- NORMAL PART -----------------------------------

#[cfg(not(feature = "embassy"))]
pub struct UniversalSPIflash<
    'a,
    SPI: SpiBus,
    EN: OutputPin,
    PWR: OutputPin,
    SS: OutputPin,
    D: DelayNs,
> {
    spi: SPI,
    pwr: PWR,
    en: EN,
    ss: SS,
    delay: &'a mut D,
}

#[cfg(not(feature = "embassy"))]
impl<'a, SPI, EN, PWR, SS, D> UniversalSPIflash<'a, SPI, EN, PWR, SS, D>
where
    SPI: SpiBus,
    EN: OutputPin,
    PWR: OutputPin,
    SS: OutputPin,
    D: DelayNs,
{
    pub fn new(spi: SPI, en: EN, pwr: PWR, ss: SS, delay: &'a mut D) -> Self {
        Self {
            spi,
            pwr,
            en,
            ss,
            delay,
        }
    }
    fn set_low_ss(&mut self) -> Result<(), FlashError> {
        self.ss.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_ss(&mut self) -> Result<(), FlashError> {
        self.ss.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
    fn set_low_pwr(&mut self) -> Result<(), FlashError> {
        self.pwr.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_pwr(&mut self) -> Result<(), FlashError> {
        self.pwr.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
    fn set_low_en(&mut self) -> Result<(), FlashError> {
        self.en.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_en(&mut self) -> Result<(), FlashError> {
        self.en.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
}

#[cfg(not(feature = "embassy"))]
impl<'a, SPI: SpiBus, EN: OutputPin, PWR: OutputPin, SS: OutputPin, D: DelayNs> Flash
    for UniversalSPIflash<'a, SPI, EN, PWR, SS, D>
{
    fn flash(&mut self, bin: &[u8]) -> Result<(), FlashError> {
        // Reset for moment
        self.set_low_pwr()?;
        self.set_high_en()?;
        self.delay.delay_ms(500);

        // Power Up FPGA
        self.set_low_ss()?;
        self.set_low_en()?;
        self.set_low_pwr()?;
        self.delay.delay_ms(100);
        self.set_high_en()?;
        self.set_high_pwr()?;
        self.delay.delay_ms(100);

        // Start SPI Interface
        self.set_high_ss()?;
        self.delay.delay_ms(2);
        self.set_low_ss()?;

        // Tranfer the data in 4096 chunks wise.
        for chunk in bin.chunks(4096) {
            self.spi
                .write(chunk)
                .map_err(|_| FlashError::SpiWriteError)?;
        }

        self.set_high_ss()?;
        self.delay.delay_ms(100);

        Ok(())
    }
}

// ------------- No Async on hal (Simply Blocking) but delay by async --------------

#[cfg(feature = "embassy")]
pub struct UniversalSPIflash<SPI: SpiBus, EN: OutputPin, PWR: OutputPin, SS: OutputPin> {
    spi: SPI,
    pwr: PWR,
    en: EN,
    ss: SS,
}

#[cfg(feature = "embassy")]
impl<SPI, EN, PWR, SS> UniversalSPIflash<SPI, EN, PWR, SS>
where
    SPI: SpiBus,
    EN: OutputPin,
    PWR: OutputPin,
    SS: OutputPin,
{
    pub fn new(spi: SPI, en: EN, pwr: PWR, ss: SS) -> Self {
        Self { spi, pwr, en, ss }
    }
    fn set_low_ss(&mut self) -> Result<(), FlashError> {
        self.ss.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_ss(&mut self) -> Result<(), FlashError> {
        self.ss.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
    fn set_low_pwr(&mut self) -> Result<(), FlashError> {
        self.pwr.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_pwr(&mut self) -> Result<(), FlashError> {
        self.pwr.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
    fn set_low_en(&mut self) -> Result<(), FlashError> {
        self.en.set_low().map_err(|_| FlashError::SetLowError)?;
        Ok(())
    }
    fn set_high_en(&mut self) -> Result<(), FlashError> {
        self.en.set_high().map_err(|_| FlashError::SetHighError)?;
        Ok(())
    }
}

#[cfg(feature = "embassy")]
impl<SPI: SpiBus, EN: OutputPin, PWR: OutputPin, SS: OutputPin> AsyncFlash
    for UniversalSPIflash<SPI, EN, PWR, SS>
{
    async fn flash(&mut self, bin: &[u8]) -> Result<(), FlashError> {
        {
            // Reset for moment

            use embassy_time::Timer;
            self.set_low_pwr()?;
            self.set_high_en()?;
            Timer::after_millis(500).await;

            // Power Up FPGA
            self.set_low_ss()?;
            self.set_low_en()?;
            self.set_low_pwr()?;
            Timer::after_millis(100).await;
            self.set_high_en()?;
            self.set_high_pwr()?;
            Timer::after_millis(100).await;

            // Start SPI Interface
            self.set_high_ss()?;
            Timer::after_millis(2).await;
            self.set_low_ss()?;

            // Tranfer the data in 4096 chunks wise.
            for chunk in bin.chunks(4096) {
                self.spi
                    .write(chunk)
                    .map_err(|_| FlashError::SpiWriteError)?;
            }

            self.set_high_ss()?;
            Timer::after_millis(100).await;

            Ok(())
        }
    }
}
