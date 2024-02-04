import type { PayloadAction } from "@reduxjs/toolkit"
import { createAppSlice } from "../../app/createAppSlice"
import {circleToAnsi, RavensGleaning} from "../../utils/ansi"

export interface GameWindowSliceState {
  chunks: string[]
}

const initialState: GameWindowSliceState = {
  chunks: []
}

export const gameWindowSlice = createAppSlice({
    name: "gameText",
    initialState,
    reducers: create => ({
        addChunk: create.reducer((state, action: PayloadAction<string>) => {
        state.chunks.push(RavensGleaning.html(circleToAnsi(action.payload)))
        }),
        clearChunks: create.reducer(state => {
        state.chunks = []
        }),
    }),
    selectors: {
        chunksCount: state => state.chunks.length,
        chunks: state => state.chunks,
    }
})

export const { addChunk, clearChunks } = gameWindowSlice.actions
export const { chunksCount, chunks } = gameWindowSlice.selectors