#include "swap_chain.h"

SwapChain::SwapChain(std::shared_ptr<Device> device)
{
	m_device = device;
}