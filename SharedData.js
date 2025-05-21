/**
 * @typedef {keyof typeof SchemaType} SchemaKey
 * @typedef {(typeof SchemaType)[SchemaKey]} SchemaType
 */
export const SchemaType = {
    u8: Uint8Array,
    i32: Int32Array,
    u32: Uint32Array,
    f32: Float32Array,
    f64: Float64Array,
};

/**
 * @typedef {Record<string, { type: SchemaKey, count: number}} Schema
 *//**
 * @typedef {{schema: T, buffer: SharedArrayBuffer}} SharedBufferData
 * @template {Schema} T
 */

/**
 * Schema builder.
 */
export class DataSchema {
    /** @param {Schema} schema  */
    static byteSize(schema) {
        return Object.values(schema).reduce((acc, el) => {
            const bytes = SchemaType[el.type].BYTES_PER_ELEMENT;
            const mod = acc % bytes;
            if (mod !== 0) {
                acc += bytes - mod;
            }
            acc += bytes * el.count;
            return acc;
        }, 0);
    }

    /**
     * @returns {SharedBufferData} The data to be sahred across workers.
     */
    build() {
        return {
            schema: this.schema,
            buffer: new SharedArrayBuffer(this.byteSize()),
        };
    }

    /**
     * Build a schema.
     * @param {T} schema
     * @template {Record<string, [SchemaKey, number] | SchemaKey>} T
     * @returns {{[P in keyof T]: T[P] extends string ? {type: T[P], count: 1} : {type: T[P][0], count: T[P][1]}}
     */
    static buildSchema(schema) {
        const result = {};
        for (const key in schema) {
            if (typeof schema[key] === "string") {
                result[key] = { type: schema[key], count: 1 };
            } else {
                result[key] = { type: schema[key][0], count: schema[key][1] };
            }
        }
        return result;
    }
    /**
     * @param {T} schema
     * @template {Parameters<typeof DataSchema.buildSchema>[0]} T
     * @returns {SharedBufferData<ReturnType<typeof DataSchema.buildSchema<T>>}
     */
    static build(schema) {
        const s = this.buildSchema(schema);
        return { buffer: new SharedArrayBuffer(this.byteSize(s)), schema: s };
    }

    /**
     * Build views around a schema.
     * @param {{schema: T, buffer: SharedArrayBuffer}} data
     * @template {Schema} T
     * @returns {{[P in keyof T]: InstanceType<(typeof SchemaType)[T[P]["type"]]>}}
     */
    static view(data) {
        let offset = 0;
        const views = {};
        for (const [name, el] of Object.entries(data.schema)) {
            const constructor = SchemaType[el.type];
            const bytes = constructor.BYTES_PER_ELEMENT;
            const mod = offset % bytes;
            if (mod !== 0) {
                offset += bytes - mod;
            }
            views[name] = new constructor(data.buffer, offset, el.count);
            offset += views[name].byteLength;
        }
        return views;
    }
}
