#include <EnginePCH.h>

#include "QueryDX11.h"

QueryDX11::QueryDX11( ID3D11Device2* pDevice, QueryType queryType, uint8_t numBuffers )
    : m_pDevice( pDevice )
    , m_QueryType( queryType )
    , m_NumBuffers( numBuffers )
{
    m_pDevice->GetImmediateContext2( &m_pDeviceContext );

    D3D11_QUERY_DESC queryDesc = {};

    switch ( m_QueryType )
    {
    case QueryType::Timer:
        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
        break;
    case QueryType::CountSamples:
        queryDesc.Query = D3D11_QUERY_OCCLUSION;
        break;
    case QueryType::CountSamplesPredicate:
        queryDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
        break;
    case QueryType::CountPrimitives:
    case QueryType::CountTransformFeedbackPrimitives:
        queryDesc.Query = D3D11_QUERY_SO_STATISTICS;
        break;
    default:
        break;
    }

    m_Queries[0].resize( m_NumBuffers );

    for ( uint8_t i = 0; i < m_NumBuffers; ++i )
    {
        if ( FAILED( m_pDevice->CreateQuery( &queryDesc, &m_Queries[0][i] ) ) )
        {
            ReportError( "Failed to create Query object." );
        }
    }

    // For timer queries, we also need to create the disjoint timer queries.
    if ( m_QueryType == Query::QueryType::Timer )
    {
        m_DisjointQueries.resize( m_NumBuffers );
        m_Queries[1].resize( m_NumBuffers );

        D3D11_QUERY_DESC disjointQueryDesc = {};
        disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

        for ( uint8_t i = 0; i < m_NumBuffers; ++i )
        {
            if ( FAILED( m_pDevice->CreateQuery( &queryDesc, &m_Queries[1][i] ) ) ||
                 FAILED( m_pDevice->CreateQuery( &disjointQueryDesc, &m_DisjointQueries[i] ) ) )
            {
                ReportError( "Failed to create timer query object." );
            }
        }
    }
}

QueryDX11::~QueryDX11()
{}

void QueryDX11::Begin( int64_t frame )
{
    int buffer = ( frame - 1L ) % m_NumBuffers;
    if ( buffer >= 0 )
    {
        if ( m_QueryType == QueryType::Timer )
        {
            m_pDeviceContext->Begin( m_DisjointQueries[buffer].Get() );
            m_pDeviceContext->End( m_Queries[0][buffer].Get() );
        }
        else
        {
            m_pDeviceContext->Begin( m_Queries[0][buffer].Get() );
        }
    }
}

void QueryDX11::End( int64_t frame )
{
    int buffer = ( frame - 1L ) % m_NumBuffers;
    if ( buffer >= 0 )
    {
        if ( m_QueryType == QueryType::Timer )
        {
            m_pDeviceContext->End( m_Queries[1][buffer].Get() );
            m_pDeviceContext->End( m_DisjointQueries[buffer].Get() );
        }
        else
        {
            m_pDeviceContext->End( m_Queries[0][buffer].Get() );
        }
    }

}

bool QueryDX11::QueryResultAvailable( int64_t frame )
{
    bool result = false;
    int buffer = ( frame - 1L ) % m_NumBuffers;

    if ( buffer >= 0 )
    {
        if ( m_QueryType == QueryType::Timer )
        {
            result = ( m_pDeviceContext->GetData( m_DisjointQueries[buffer].Get(), nullptr, 0, 0 ) == S_OK );
        }
        else
        {
            result = ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), nullptr, 0, 0 ) == S_OK );
        }
    }

    return result;
}

Query::QueryResult QueryDX11::GetQueryResult( int64_t frame )
{
    QueryResult result = {};
    int buffer = ( frame - 1L ) % m_NumBuffers;

    if ( buffer >= 0 )
    {
        if ( m_QueryType == QueryType::Timer )
        {
            while ( m_pDeviceContext->GetData( m_DisjointQueries[buffer].Get(), nullptr, 0, 0 ) == S_FALSE )
            {
                Sleep( 1L );
            }
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timeStampDisjoint;
            m_pDeviceContext->GetData( m_DisjointQueries[buffer].Get(), &timeStampDisjoint, sizeof( D3D11_QUERY_DATA_TIMESTAMP_DISJOINT ), 0 );
            if ( timeStampDisjoint.Disjoint == FALSE )
            {
                UINT64 beginTime, endTime;
                if ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), &beginTime, sizeof( UINT64 ), 0 ) == S_OK &&
                     m_pDeviceContext->GetData( m_Queries[1][buffer].Get(), &endTime, sizeof( UINT64 ), 0 ) == S_OK )
                {
                    result.ElapsedTime = ( endTime - beginTime ) / double( timeStampDisjoint.Frequency );
                    result.IsValid = true;
                }
            }
        }
        else
        {
            // Wait for the results to become available.
            while ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), nullptr, 0, 0 ) )
            {
                Sleep( 1L );
            }

            switch ( m_QueryType )
            {
            case QueryType::CountSamples:
            {
                UINT64 numSamples = 0;
                if ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), &numSamples, sizeof( UINT64 ), 0 ) == S_OK )
                {
                    result.NumSamples = numSamples;
                    result.IsValid = true;
                }
            }
            break;
            case Query::QueryType::CountSamplesPredicate:
            {
                BOOL anySamples = FALSE;
                if ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), &anySamples, sizeof( UINT64 ), 0 ) == S_OK )
                {
                    result.AnySamples = anySamples == TRUE;
                    result.IsValid = true;
                }
            }
            break;
            case Query::QueryType::CountPrimitives:
            case Query::QueryType::CountTransformFeedbackPrimitives:
            {
                D3D11_QUERY_DATA_SO_STATISTICS streamOutStats = {};
                if ( m_pDeviceContext->GetData( m_Queries[0][buffer].Get(), &streamOutStats, sizeof( D3D11_QUERY_DATA_SO_STATISTICS ), 0 ) == S_OK )
                {
                    result.PrimitivesGenerated = result.TransformFeedbackPrimitives = streamOutStats.NumPrimitivesWritten;
                    result.IsValid = true;
                }
            }
            break;
            default:
                break;
            }
        }
    }

    return result;
}

uint8_t QueryDX11::GetBufferCount() const
{
    return m_NumBuffers;
}
