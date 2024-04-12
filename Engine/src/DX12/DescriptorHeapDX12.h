#pragma once
/// A structure that wraps a descriptor heap.

#include <d3d12.h>

class RenderDeviceDX12;

template< typename HandleType >
class DescriptorHandleDX12
{
public:
    // Copy constructor.
    DescriptorHandleDX12( const DescriptorHandleDX12<HandleType>& copy );

    /**
    * Implicit conversion operator to convert to D3D12 descriptor handle.
    * @return The current location in the descriptor heap this handle is pointing.
    */
    operator HandleType() const;

    /**
     * Increment the handle to the next point in the heap.
     */
    DescriptorHandleDX12<HandleType>& operator++(); // Pre-increment operator.
    DescriptorHandleDX12<HandleType> operator++( int ); // Post-increment operator.

    /**
     * Decrement the handle to the previous point in the heap.
     */
    DescriptorHandleDX12<HandleType>& operator--(); // Pre-decrement operator.
    DescriptorHandleDX12<HandleType> operator--( int ); // Post-decrement operator.

    /**
     * Increment the handle by a specific amount.
     */
    DescriptorHandleDX12<HandleType>& operator+=( UINT count );
    DescriptorHandleDX12<HandleType> operator+( UINT count ) const;

    /**
     * Decrement the handle by a specific amount.
     */
    DescriptorHandleDX12<HandleType>& operator-=( UINT count );
    DescriptorHandleDX12<HandleType> operator-( UINT count ) const;

    /**
     * Set the handle to a specific location in the heap (relative to the start of the heap).
     * Set the handle to 0 to reset the handle to the beginning of the heap.
     */
    DescriptorHandleDX12<HandleType>& operator=( UINT offset );

private:
    friend class DescriptorHeapDX12;

    // Can only be created by a descriptor heap object.
    DescriptorHandleDX12();
    DescriptorHandleDX12( HandleType handle, UINT increment );

    // A handle to the beginning of the heap for which this handle was created.
    // This should not change during the lifetime of the heap.
    HandleType m_StartOfHeap;
    // The current location in the heap that this handle is pointing.
    HandleType m_CurrentLocation;
    // The size of a single descriptor in the descriptor heap.
    UINT m_Increment;
};

typedef DescriptorHandleDX12<D3D12_CPU_DESCRIPTOR_HANDLE> CPUDescriptorHandleDX12;
typedef DescriptorHandleDX12<D3D12_GPU_DESCRIPTOR_HANDLE> GPUDescriptorHandleDX12;

class DescriptorHeapDX12
{
public:
    /**
     * Create a descriptor heap.
     * @param[in] pDevice   D3D12Device pointer that will be used to create the descriptor heap.
     * @param[in] type      The type of descriptor heap to create.
     * @param[in] count     The number of descriptors that this heap can store. Default is 1.
     *
     * @see https://msdn.microsoft.com/en-us/library/dn859379(v=vs.85).aspx
     */
    DescriptorHeapDX12( ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1 );

    /**
     * Get the CPU descriptor heap handle.
     * This will be the incremented handle.
     * To get a handle to the start of the heap use @see GetCPUHandleStart()
     */
    CPUDescriptorHandleDX12& GetCPUHandle();
    CPUDescriptorHandleDX12 GetCPUHandleStart() const;

    /**
     * Get the GPU descriptor heap handle.
     * This will be the incremented handle.
     * To get a handle to the start of the heap use @see GetGPUHandleStart();
     */
    GPUDescriptorHandleDX12& GetGPUHandle();
    GPUDescriptorHandleDX12 GetGPUHandleStart() const;

protected:

private:
    // Don't allow default creation.
    DescriptorHeapDX12();
    // Don't allow copies.
    DescriptorHeapDX12( const DescriptorHeapDX12& copy );
    // Don't allow assignment.
    const DescriptorHeapDX12& operator=( const DescriptorHeapDX12& other );

    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    UINT m_Increment;
    UINT m_Size;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pHeap;
    CPUDescriptorHandleDX12 m_CPUHandle;
    GPUDescriptorHandleDX12 m_GPUHandle;
};

#include "DescriptorHeapDX12.inl"