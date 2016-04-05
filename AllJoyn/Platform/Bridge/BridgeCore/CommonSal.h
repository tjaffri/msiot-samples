//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//

#pragma once

// SAL Annotations do not exist in iOS and Android so we
// re-define them here to mean nothing for those platforms
#if defined(__ANDROID_API__) || defined (__APPLE__)

#define _Use_decl_annotations_

#define _In_
#define _In_opt_
#define _Inout_
#define _Inout_count_(size)
#define _Inout_opt_
#define _Inout_updates_(size)
#define _In_z_
#define _In_opt_z_
#define _In_reads_(size)
#define _In_reads_opt_(size)
#define __in_bcount(size)
#define __in
#define __inout_bcount(size)

#define _Out_
#define _Out_opt_
#define _Out_writes_(size)
#define _Out_writes_all_(size)
#define _Out_writes_opt_(size)
#define _Out_writes_z_
#define _Out_writes_bytes_to_opt_
#define _Out_writes_bytes_(size)
#define __out_bcount_full(size)

#define __fallthrough

#endif
