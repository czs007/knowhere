/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <faiss/gpu/GpuIndexIVFSQHybrid.h>
#include <faiss/gpu/GpuIndexFlat.h>
#include <faiss/gpu/GpuResources.h>
#include <faiss/gpu/impl/GpuScalarQuantizer.cuh>
#include <faiss/gpu/impl/IVFFlat.cuh>
#include <faiss/gpu/utils/CopyUtils.cuh>
#include <faiss/gpu/utils/DeviceUtils.h>
#include <faiss/utils/utils.h>
#include <limits>

namespace faiss {
namespace gpu {

GpuIndexIVFSQHybrid::GpuIndexIVFSQHybrid(
        GpuResourcesProvider* provider,
        faiss::IndexIVFSQHybrid* index,
        GpuIndexIVFSQHybridConfig config)
        : GpuIndexIVF(
                  provider,
                  index->d,
                  index->metric_type,
                  index->metric_arg,
                  index->nlist,
                  config),
          ivfSQConfig_(config),
          sq(index->sq),
          by_residual(index->by_residual),
          reserveMemoryVecs_(0),
          index_(nullptr) {
    gpu::GpuIndexFlat* quantizer = nullptr;
    copyFrom(index, quantizer, 0);

    FAISS_THROW_IF_NOT_MSG(isSQSupported(sq.qtype),
                           "Unsupported QuantizerType on GPU");
}

GpuIndexIVFSQHybrid::GpuIndexIVFSQHybrid(
        GpuResourcesProvider* provider,
        int dims,
        int nlist,
        faiss::QuantizerType qtype,
        faiss::MetricType metric,
        bool encodeResidual,
        GpuIndexIVFSQHybridConfig config)
        : GpuIndexIVF(provider, dims, metric, 0, nlist, config),
          ivfSQConfig_(config),
          sq(dims, qtype),
          by_residual(encodeResidual),
          reserveMemoryVecs_(0),
          index_(nullptr) {

    // faiss::Index params
    this->is_trained = false;

    // We haven't trained ourselves, so don't construct the IVFFlat index yet
    FAISS_THROW_IF_NOT_MSG(isSQSupported(sq.qtype),
                           "Unsupported QuantizerType on GPU");
}

GpuIndexIVFSQHybrid::~GpuIndexIVFSQHybrid() {
}

void GpuIndexIVFSQHybrid::reserveMemory(size_t numVecs) {
    reserveMemoryVecs_ = numVecs;
    if (index_) {
        DeviceScope scope(config_.device);
        index_->reserveMemory(numVecs);
    }
}

void GpuIndexIVFSQHybrid::copyFrom(const faiss::IndexIVFSQHybrid* index) {
    DeviceScope scope(config_.device);

    // Clear out our old data
    index_.reset();

    // Copy what we need from the CPU index
    GpuIndexIVF::copyFrom(index);
    sq = index->sq;
    by_residual = index->by_residual;

    // The other index might not be trained, in which case we don't need to copy
    // over the lists
    if (!index->is_trained) {
        return;
    }

    // Otherwise, we can populate ourselves from the other index
    this->is_trained = true;

    // Copy our lists as well
    index_.reset(new IVFFlat(resources_.get(),
                             quantizer->getGpuData(),
                             index->metric_type,
                             index->metric_arg,
                             by_residual,
                             &sq,
                             ivfSQConfig_.interleavedLayout,
                             ivfSQConfig_.indicesOptions,
                             config_.memorySpace));
    if (ReadOnlyArrayInvertedLists* rol =
                dynamic_cast<ReadOnlyArrayInvertedLists*>(index->invlists)) {
      index_->copyCodeVectorsFromCpu(
                (const float*)(rol->pin_readonly_codes->data),
                (const idx_t*)(rol->pin_readonly_ids->data),
                rol->readonly_length);
    } else {
        // Copy all of the IVF data
        index_->copyInvertedListsFrom(index->invlists);
    }
}

void GpuIndexIVFSQHybrid::copyFrom(
        faiss::IndexIVFSQHybrid* index,
        gpu::GpuIndexFlat*& qt,
        long mode) {
    DeviceScope scope(config_.device);

    // Clear out our old data
    index_.reset();

    GpuIndexIVF::copyFrom(index, qt, mode);
    if (mode == 1) {
        // Only copy quantizer
        return ;
    }

    sq = index->sq;
    by_residual = index->by_residual;

    // The other index might not be trained, in which case we don't need to copy
    // over the lists
    if (!index->is_trained) {
        return;
    }

    // Otherwise, we can populate ourselves from the other index
    this->is_trained = true;

    // Copy our lists as well
    index_.reset(new IVFFlat(resources_.get(),
                             quantizer->getGpuData(),
                             index->metric_type,
                             index->metric_arg,
                             by_residual,
                             &sq,
                             ivfSQConfig_.interleavedLayout,
                             ivfSQConfig_.indicesOptions,
                             config_.memorySpace));
    if (ReadOnlyArrayInvertedLists* rol =
                dynamic_cast<ReadOnlyArrayInvertedLists*>(index->invlists)) {
        index_->copyCodeVectorsFromCpu(
                (const float *)(rol->pin_readonly_codes->data),
                (const idx_t *)(rol->pin_readonly_ids->data),
                rol->readonly_length);
    } else {
        // Copy all of the IVF data
        index_->copyInvertedListsFrom(index->invlists);
    }
}

void GpuIndexIVFSQHybrid::copyTo(faiss::IndexIVFSQHybrid* index) const {
    DeviceScope scope(config_.device);

    // We must have the indices in order to copy to ourselves
    FAISS_THROW_IF_NOT_MSG(
            ivfSQConfig_.indicesOptions != INDICES_IVF,
            "Cannot copy to CPU as GPU index doesn't retain "
            "indices (INDICES_IVF)");

    GpuIndexIVF::copyTo(index);
    index->sq = sq;
    index->by_residual = by_residual;
    index->code_size = sq.code_size;

    InvertedLists* ivf = new ArrayInvertedLists(nlist, index->code_size);
    index->replace_invlists(ivf, true);

    if (index_) {
        // Copy IVF lists
        index_->copyInvertedListsTo(ivf);
    }
}

size_t GpuIndexIVFSQHybrid::reclaimMemory() {
    if (index_) {
        DeviceScope scope(config_.device);
        return index_->reclaimMemory();
    }
    return 0;
}

void GpuIndexIVFSQHybrid::reset() {
    if (index_) {
        DeviceScope scope(config_.device);

        index_->reset();
        this->ntotal = 0;
    } else {
        FAISS_ASSERT(this->ntotal == 0);
    }
}

int GpuIndexIVFSQHybrid::getListLength(int listId) const {
    FAISS_ASSERT(index_);
    DeviceScope scope(config_.device);
    return index_->getListLength(listId);
}

std::vector<uint8_t> GpuIndexIVFSQHybrid::getListVectorData(
        int listId,
        bool gpuFormat) const {
    FAISS_ASSERT(index_);
    DeviceScope scope(config_.device);
    return index_->getListVectorData(listId, gpuFormat);
}

std::vector<Index::idx_t> GpuIndexIVFSQHybrid::getListIndices(
        int listId) const {
    FAISS_ASSERT(index_);
    DeviceScope scope(config_.device);
    return index_->getListIndices(listId);
}

void GpuIndexIVFSQHybrid::trainResiduals_(Index::idx_t n, const float* x) {
    // The input is already guaranteed to be on the CPU
    sq.train_residual(n, x, quantizer, by_residual, verbose);
}

void GpuIndexIVFSQHybrid::train(Index::idx_t n, const float* x) {
    // For now, only support <= max int results
    FAISS_THROW_IF_NOT_FMT(n <= (Index::idx_t) std::numeric_limits<int>::max(),
                           "GPU index only supports up to %d indices",
                           std::numeric_limits<int>::max());

    DeviceScope scope(config_.device);

    if (this->is_trained) {
        FAISS_ASSERT(quantizer->is_trained);
        FAISS_ASSERT(quantizer->ntotal == nlist);
        FAISS_ASSERT(index_);
        return;
    }

    FAISS_ASSERT(!index_);

    // FIXME: GPUize more of this
    // First, make sure that the data is resident on the CPU, if it is not on
    // the CPU, as we depend upon parts of the CPU code
    auto hostData = toHost<float, 2>(
            (float*)x,
            resources_->getDefaultStream(config_.device),
            {(int) n, (int) this->d});

    trainQuantizer_(n, hostData.data());
    trainResiduals_(n, hostData.data());

    // The quantizer is now trained; construct the IVF index
    index_.reset(new IVFFlat(resources_.get(),
                             quantizer->getGpuData(),
                             this->metric_type,
                             this->metric_arg,
                             by_residual,
                             &sq,
                             ivfSQConfig_.interleavedLayout,
                             ivfSQConfig_.indicesOptions,
                             config_.memorySpace));

    if (reserveMemoryVecs_) {
        index_->reserveMemory(reserveMemoryVecs_);
    }

    this->is_trained = true;
}

void GpuIndexIVFSQHybrid::addImpl_(
        int n,
        const float* x,
        const Index::idx_t* xids) {
    // Device is already set in GpuIndex::add
    FAISS_ASSERT(index_);
    FAISS_ASSERT(n > 0);

    // Data is already resident on the GPU
    Tensor<float, 2, true> data(const_cast<float*>(x), {n, (int) this->d});
    Tensor<Index::idx_t, 1, true> labels(const_cast<Index::idx_t*>(xids), {n});

    // Not all vectors may be able to be added (some may contain NaNs etc)
    index_->addVectors(data, labels);

    // but keep the ntotal based on the total number of vectors that we
    // attempted to add
    ntotal += n;
}

void GpuIndexIVFSQHybrid::searchImpl_(
        int n,
        const float* x,
        int k,
        float* distances,
        Index::idx_t* labels,
        const BitsetView bitset) const {
    // Device is already set in GpuIndex::search
    FAISS_ASSERT(index_);
    FAISS_ASSERT(n > 0);

    auto stream = resources_->getDefaultStream(config_.device);

    // Data is already resident on the GPU
    Tensor<float, 2, true> queries(const_cast<float*>(x), {n, (int) this->d});
    Tensor<float, 2, true> outDistances(distances, {n, k});
    Tensor<Index::idx_t, 2, true>
            outLabels(const_cast<Index::idx_t*>(labels), {n, k});

    if (!bitset) {
        auto bitsetDevice = toDeviceTemporary<uint8_t, 1>(
                resources_.get(),
                config_.device,
                nullptr,
                stream,
                {0});
        index_->query(
                queries,
                bitsetDevice,
                nprobe,
                k,
                outDistances,
                outLabels);
    } else {
        auto bitsetDevice = toDeviceTemporary<uint8_t, 1>(
                resources_.get(),
                config_.device,
                const_cast<uint8_t*>(bitset.data()),
                stream,
                {(int)bitset.byte_size()});
        index_->query(
                queries,
                bitsetDevice,
                nprobe,
                k,
                outDistances,
                outLabels);
    }
}

} // namespace gpu
} // namespace faiss
